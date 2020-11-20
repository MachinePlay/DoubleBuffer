#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>


namespace inf {
namespace utils {
/** 
 * @class DoubleData.
 * @brief double buffer data, DataType must implement reload function.
 **/
template <typename DataType>
class DoubleData {
public:
    /* ctor. */
    DoubleData() = default;

    /* dtor. */
    virtual ~DoubleData() = default;

    template <typename... Args>
    int init(Args&&... args) {
        _current_ptr = std::make_shared<DataType>(std::forward<Args>(args)...);
        _pre_ptr = std::make_shared<DataType>(std::forward<Args>(args)...);
        return 0;
    }
    
    /* get on-using data. */
    std::shared_ptr<DataType> get_data() const {
        return _current_ptr;
    }

    /* whether the backup ptr is in use. */
    bool is_allow_reload() {
        if (_pre_ptr == nullptr) {
            return true;
        }
        return _pre_ptr.use_count() > 1 ? false : true;
    }

    /**
    * reload the dual buffer
    * @param DataType relaod parameter, DataType must implement reload function
    * @return true if reload ok, otherwise false.
    */
   template <typename... Args>
   bool reload(Args&&... args) {
        //only one thread can reload
       std::lock_guard<std::mutex> lock(_lock);
       if(is_allow_reload()) {
            _pre_ptr->reload(std::forward<Args>(args)...);
            swap_dual_buffer();
            return true;
       }

        return false;
   }

    /**
    * get dual buffer backup data
    * get the backup data, change the buffer and swap this buffer to front
    * @return std::shared_ptr<DataType> 
    */
   std::shared_ptr<DataType> get_backup_data() {
        return _pre_ptr;
   }

    /**
    * swap the current buffer 
    */
   void swap_dual_buffer() {
       std::lock_guard<std::mutex> lock(_lock);
       _current_ptr.swap(_pre_ptr);
   }

    
    /* none-copy. */
    DoubleData(const DoubleData &rhs) = delete;
    DoubleData &operator=(const DoubleData &rhs) = delete;
    
private:
    /* current data ptr. */
    std::shared_ptr<DataType> _current_ptr;
    /* previous data ptr. */
    std::shared_ptr<DataType> _pre_ptr;
    /* reload and writer lock. */
    std::mutex _lock;
};
} // end namespace utils
} // end namespace inf
