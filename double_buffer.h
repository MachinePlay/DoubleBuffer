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

namespace inf {
namespace utils {

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
            if(file_name.empty() && stat(file_name.c_str(), &file_status) != 0) {
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

    /* ctor. */
    DoubleData(LoaderPtr loader) : _loader(std::move(loader)) {
        
    };
    /* dtor. */
    virtual ~DoubleData() = default;
    
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

        return 0;
    }

    /**
    * change the backup and front data
    * @return 
    */
    bool swap_data() {
        //only one thread can manipulate
        std::lock_guard<std::mutex> lock(_lock);
        //make sure the backup data has no user.
        if (_backup.use_count() > 1) {
            return false;
        }
        *_backup = _loader->load();
        _current.swap(_backup);
        return true;
        
    }


    /**
    * get current data ptr, using this data on front.
    * @return std::shared_ptr<BufferType> 
    */
    BufferPtr get_current() {
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

    /* load funtion, always load latest file, and parse data. */
    YAML::Node load() {
        return YAML::LoadFile(_config_file_name.c_str());
    }

    
private:
    YamlLoader(const YamlLoader &rhs) = delete;
    YamlLoader &operator=(const YamlLoader &rhs) = delete;
    std::string _config_file_name {""};
};

} // end namespace utils
} // end namespace inf 
