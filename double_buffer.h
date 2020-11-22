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
#include <thread>
#include <utility>
#include <mutex>


namespace inf {
namespace utils {

// class ConfManager {
// public:
//     ConfManager() = default;
//     virtual ~ConfManager() = default;

//     static ConfManager& get_instance() {
//         static ConfManager instance;
//         return instance;
//     }
//     int init(const std::string& file_name) {
        
//     }

// private:
//     typedef std::unordered_map<std::string, std::string> ConfMap;
//     typedef std::unique_ptr<ConfMap>   ConfMapPtr;
//     typedef std::unordered_map<std::string, ConfMapPtr> ConfTable;
    
//     ConfManager(const ConfManager &rhs) = delete;
//     ConfManager &operator=(const ConfManager &rhs) = delete;

//     std::string     _file_name;
//     ConfTable       _conf_file_table;
//     static std::mutex      _file_lock;
// };

// /** 
//  * @class SwitchMonitor.
//  * moniter the file info, and decicde wether the config file need reload
//  **/
// class SwitchMonitor {

// };

/** 
 * @class DoubleData.
 * double data has the BuferType and it's own loader
 * Loader must implement realod function.
 **/
template <typename BufferType, typename Loader>
class DoubleData {
public:
    typedef std::shared_ptr<BufferType> BufferPtr;
    typedef std::unique_ptr<Loader>     LoaderPtr;

    /* ctor. */
    DoubleData(LoaderPtr loader) : _loader(loader) {
        
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
        *_current = _loader.load();
        *_backup = *_current;

        return 0;
    }

    /**
    * change the backup and front data
    * @return 
    */
    bool swap_data() {
        std::lock_guard<std::mutex> lock(_lock);
        if (_backup.use_count() > 1) {
            return false;
        }
        _backup = _loader.load();
        _current.swap(_backup);
        return true;
        
    }

    // /**
    // * reload data
    // * @param 
    // * @return 
    // */


    /**
    * get current data ptr, using this data on front.
    * @return std::shared_ptr<BufferType> 
    */
    BufferPtr get_current() {
        return _current;
    }

    /* get backup data. */
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
    int init(const std::string &file_name) {
        _config_file_name = file_name;
        //wether the file exists
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
