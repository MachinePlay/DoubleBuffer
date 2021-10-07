#pragma once
#include "yaml-cpp/yaml.h"
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <unordered_map>
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utility>
#include <mutex>
#include <iostream>
#include <pthread.h>

namespace inf {
namespace utils {

const int64_t DEFAULT_INTERVAL = 3;

/** 
 * @class SwitchMonitor.
 * moniter the file info, and decicde wether the config file need reload
 **/
class SwitchMonitor {
public:
    /* ctor. */
    SwitchMonitor() = default;
    virtual ~SwitchMonitor() = default;
    
    /**
    * init the monitor file name
    * monite the file status.
    * @param std::string file_name
    * @return bool true: ok, false: failed
    */
    virtual bool init(const std::string& file_name) {
        if (file_name.empty()) {
            return false;
        }
        struct stat file_status;
        if (file_name.empty() || stat(file_name.c_str(), &file_status) != 0) {
            return false;
        }
        _file_status_table.insert(std::make_pair(file_name, file_status.st_mtime));

        return true; 
    }
    
    /**
    * init the monitor file name, batch version
    * @param std;:vector<std::string> file_names
    * @return bool true: ok, false: failed
    */
    virtual bool init(const std::vector<std::string>& file_names) {
        if (file_names.empty()) {
            return false;
        }
        struct stat file_status;
        for (auto &file_name : file_names) {
            if (file_name.empty() && stat(file_name.c_str(), &file_status) != 0) {
                return false;
            }
            _file_status_table.insert(std::make_pair(file_name, file_status.st_mtime));
        }
        
        return true;
    }

    /**
    * get monitor file list
    * @return std::unoredred_map<std::string> file list 
    */
    std::unordered_map<std::string, time_t> get_monitor_file_list() const {
        return _file_status_table;
    }

    /**
    * whether the buffer need to be relaod.
    * @return std::vector<std::string> file lists need to update
    */
   std::vector<std::string> get_need_switch_file() {
       std::vector<std::string> update_file_names;
       for (auto &[file_name, last_modify_time] : _file_status_table) {
           struct stat file_status;
           if (stat(file_name.c_str(), &file_status) !=0) {
                return update_file_names;
           }

           if (file_status.st_mtime > last_modify_time) {
                update_file_names.push_back(file_name);
                last_modify_time = file_status.st_mtime;
           }
       }

       return update_file_names;
   }
    
private:
    /* file status map. */
    std::unordered_map<std::string, time_t> _file_status_table; 
};

/** 
 * @class DoubleData.
 * double data has the BuferType and it's own loader
 * Loader must implement realod() function to do specific things.
 **/
template <typename BufferType, typename Loader>
class DoubleData {
public:
    typedef std::shared_ptr<BufferType> BufferPtr;
    typedef std::unique_ptr<Loader>     LoaderPtr;
    typedef std::unique_ptr<std::thread> ThreadPtr;
    /* ctor. */
    DoubleData(LoaderPtr loader, int64_t interval = DEFAULT_INTERVAL, bool is_monitor = true) 
        : _loader(std::move(loader)), _interval(interval), _is_monitor(is_monitor) {
        
    };

    /* dtor. */
    virtual ~DoubleData() {
        if (_is_monitor && _monitor_thread && _monitor_thread->joinable()) {
                _monitor_thread->join();
        }
    }
    
    /**
    * init the double buffer, using Loader to load the buffer.
    * @return 0:ok, -1:failed.
    */
    int init() {
        std::lock_guard<std::mutex> lock(_lock);
        _current = std::make_shared<BufferType>();
        _backup  = std::make_shared<BufferType>();
        *_current = _loader->load();
        *_backup = *_current;

        _monitor.init(_loader->get_load_file_name());
        if (_is_monitor) {
            _monitor_thread.reset(new std::thread(&DoubleData::run, this));
        }
      

        return 0;
    }

    void run () {
        //detect file per interval
        while(_is_monitor) {
            auto update_files = _monitor.get_need_switch_file();
            if (!update_files.empty()){
                swap_data();

                std::cout << "swith file now " << std::endl;
            }
            sleep(_interval);
        }
    } 

    /**
    * change the backup and front data
    * @return 
    */
    bool swap_data() {
        //only one thread can manipulate
        std::lock_guard<std::mutex> lock(_lock);
        //make sure the backup data has no user.
        std::cout << "backup use_cunt " <<  _backup.use_count() << std::endl;
        std::cout << "font user_count " << _current.use_count() << std::endl;
        // if (_backup.use_count() > 1) {
        //     return false;
        // }
        std::cout << "swap start" << std::endl;
        *_backup = _loader->load();
        _current.swap(_backup);
        return true;
        
    }


    /**
    * get current data ptr, using this data on front.
    * @return std::shared_ptr<BufferType> 
    */
    BufferPtr get_current() {
        std::lock_guard<std::mutex> lock(_lock);
        return _current;
    }

    /* get backup data. this interface is only use for test, 
       DO NOT USE IT IN BUSSINESS!!!*/
    BufferPtr get_backup() {
        return _backup;
    }

    

    
private:
    /* none-copy. */
    DoubleData(const DoubleData &rhs)  = delete;
    DoubleData &operator=(const DoubleData &rhs) = delete;

    /* front data ptr, always using this ptr. */
    BufferPtr           _current;
    /* backup data ptr, update this data. */
    BufferPtr           _backup;
    /* swap front and backup data lock. */
    std::mutex          _lock;
    /* data loader. Loader must implement load() and return std::shared_ptr<BufferType> */
    LoaderPtr           _loader;
    /* whether use auto update. */
    std::atomic<bool>   _is_monitor;
    /* file monitor interval. */
    int64_t             _interval{60};
    /* file SwitchMonitor. need to be init afer DoubleBuffer init()*/
    SwitchMonitor       _monitor;
    /* thread ptr. monitor the conf file. */
    ThreadPtr           _monitor_thread;    
};



/** 
 * @class YamlLoader.
 * an Loader example
 * load from yaml file, it's a recommend way to implement init and load funtion to use double buffer.
 * init() function only load the config file name
 * load() funciont parse the config file and return BufferType
 **/
class YamlLoader {
public:
    /* ctor. */
    YamlLoader() = default;
    virtual ~YamlLoader() = default;

    /* init function, read file from yaml. */
    bool init(const std::string &file_name) {
        if (file_name.empty()) {
            return false;
        }
        _config_file_name = file_name;
        //whether the file exists
        struct stat file_stat;
        if (stat(file_name.c_str(), &file_stat) != 0) {
            return false;
        }
        return true;
    }

    /* config file name, user relative path*/
    std::string get_load_file_name() const {
        return _config_file_name;
    }

    /* load funtion, always load latest file, and parse data. */
    YAML::Node load() {
        return YAML::LoadFile(_config_file_name.c_str());
    }

    
private:
    YamlLoader(const YamlLoader &rhs) = delete;
    YamlLoader &operator=(const YamlLoader &rhs) = delete;
    std::string _config_file_name {""};
};


//not implement now
class ConfManager {
public:
    ConfManager() = default;
    virtual ~ConfManager() = default;

    static ConfManager& get_instance() {
        static ConfManager instance;
        return instance;
    }

    /**
     * init conf file by file name
     * not implement now.
     * @param std::vector<std::string> file_names
     * @return -1:Failed, 0:OK
     */
    int init(const std::string& file_names) {
        if (file_names.empty()) {
            return -1;
        }
        
        return 0;
    }
    

private:
    typedef std::unordered_map<std::string, std::string>    ConfMap;
    typedef std::unique_ptr<ConfMap>                        ConfMapPtr;
    typedef std::unordered_map<std::string, ConfMapPtr>     ConfTable;
    typedef std::unique_ptr<std::thread>                    ThreadPtr;
    
    ConfManager(const ConfManager &rhs) = delete;
    ConfManager &operator=(const ConfManager &rhs) = delete;

    std::string             _file_name;
    ConfTable               _conf_file_table;
    static std::mutex       _file_lock;
    ThreadPtr               _monitor_thread;

};


//rw lock implement
class RW_LOCK {
private:
    /* ctor. */
    RW_LOCK() {
        _rw_lock = PTHREAD_RWLOCK_INITIALIZER;
    }

    /* dtor. */
    virtual ~RW_LOCK() {
        pthread_rwlock_destroy(&_rw_lock);
    }

    /* read lock, multi thread can read without data race. */
    int read_lock() {
        return pthread_rwlock_rdlock(&_rw_lock);
    }

    /* write lock, only one thread can hold this lock. when the lock is locked, read locks and other write locks will be blocked. */
    int write_lock() {
        return pthread_rwlock_wrlock(&_rw_lock);
    }

    int unlock() {
        return pthread_rwlock_unlock(&_rw_lock);
    }

    
private:
    pthread_rwlock_t _rw_lock;
};




/** 
 * @class SopeLock Wrapper.
 **/
 template <typename LockType>
 class ScopeLock {
    /* ctor. */
    ScopeLock(); 
    /* dtor. */
    virtual ~ScopeLock();
 };

/** 
 * @class ReadLockWrapper.
 **/
template <typename LockType>
class ReadLockWrapper : public ScopeLock<LockType>{
public:
    /* ctor. */
    ReadLockWrapper() {
        _rw_lock.read_lock();
    }

    virtual ~ReadLockWrapper() {
        _rw_lock.unlock();
    }

private:
    LockType _rw_lock;
};

/** 
 * @class WriteLockWrapper.
 **/
template <typename LockType>
class WriteLockWrapper : public ScopeLock<LockType> {
    /* ctor. */
    WriteLockWrapper() {
        _rw_lock.write_lock();
    }

    /* ctor. */
    virtual ~WriteLockWrapper() {
        _rw_lock.unlock();
    }

private:
    LockType _rw_lock;
};

} // end namespace utils
} // end namespace inf 
