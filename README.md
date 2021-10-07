DoubleBuffer
===========
![GitHub](https://img.shields.io/github/license/machineplay/DoubleBuffer)
[![UnitTest](https://github.com/MachinePlay/DoubleBuffer/actions/workflows/docker-image.yml/badge.svg)](https://github.com/MachinePlay/DoubleBuffer/actions/workflows/docker-image.yml) 

A lite-weight double buffer C++ library implementation.

# 什么是DoubleBuffer
线上服务中，经常会有这样的需求：多个线程高频率去读一个数据，这个数据偶尔需要进行更新。  
大部分时间这是一个读远多于写的数据结构：配置文件的读取，每次请求到来，读取配置文件，配置更新时能够实现热加载，读取到最新的数据。  
对于这种数据结构，一个常见的做法是加读写锁，当读取的临界区比较大时，有可能阻塞读操作，开销比较高。  
另一种常见的做法是用双缓冲（DoubleBuffer）：
- 数据分为`使用中current`和`未使用backup`，存放两个buffer
- 业务只使用工作在`使用中current`的buffer
- 需要修改数据时，由一个写线程修改`未使用backup`的buffer，将其更新为最新版本，然后交换前后台数据（通常是直接将业务用的数据指针更新指向`backup`的buffer），睡眠一段时间（可以不睡,这里可以思考下为什么要睡眠），此时`current`里是最新数据，`backup`中的buffer已经算是旧数据了，可以将此时的`backup`buffer更新为`current`buffer,方便下一轮修改

# 快速使用
- 这是一个Header-Only的库，拷贝`double_buffer.h`到工程中使用即可
- 也直接使用yaml配置版本，需要链接yaml-cpp, 也可以继承后实现自己的数据relaod.   

`example`中有使用示例，使用cmake拉取yaml-cpp联编.
```
cd example
mkdir build
cmake ..
make -j8
./DoubleBufferExample
```  
即可执行，会在`build目录下新建测试用的配置文件`test.yaml`,修改文件内容，程序读取的buffer会发生改变，实现运行时自动监听文件内容变化，实现热加载

```
#include "double_buffer.h"
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <atomic>

int main() {
    //配置文件名
    const std::string config_file = "test.yaml";
    //新建配置文件
    int fd = open(config_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 777);
    std::string content= "redis_domain: 10.0.0.10";
    write(fd, content.c_str(), content.size());
    close(fd);
    //这里的作用是新建配置文件供测试使用，实际业务中的配置文件可以自己搞

    //use loader, init loader
    //用unique_ptr创建Loader，可以实现自己的loader，做自定义类似配置解析的工作
    std::unique_ptr<::inf::utils::YamlLoader> loader = std::make_unique<::inf::utils::YamlLoader>();
    //绑定loader到配置文件上
    loader->init(config_file.c_str());

    //init double data
    //初始化double buffer
    auto config_data = new ::inf::utils::DoubleData<YAML::Node, ::inf::utils::YamlLoader>(std::move(loader));
    config_data->init();
    

    //use double data
    //在业务中使用，程序运行过程中，配置文件变更，下一次会读到最新的配置
    std::atomic<int> cnt = 0;
    while(true) {
        ++cnt;
        auto config_ptr = config_data->get_current();
        std::cout << "ptr1 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr2 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr3 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr4 "<<(*config_ptr)["redis_domain"] << std::endl;
        std::cout << "ptr5 "<<(*config_ptr)["redis_domain"] << std::endl;
        //do something
        sleep(1);
        std::cout << "ptr6 "<<(*config_ptr)["redis_domain"] << std::endl;
        
        if(cnt % 2 == 0) {
            sleep(1);
        }

    }


    
    std::cout << "Hello world" << std::endl;
    return 0;
}

```



# 设计思路
## 简陋版本  
我们可以轻松的想到可以切换的数据结构，同一份数据写两个buffer，通过一个暴露在外的指针对外提供访问  
双buffer中front buffer提供给业务使用, backup buffer用来加载新数据，数据加载完成后，切换front和backup，只需要实现get_current,get_backup,swap_buffer  
- 多处写入backup的情况下，不能swap，所以要保证没有其他线程/地方调用backup buffer的时候swap
- 可以想到，如果只提供上面三个接口，reload需要业务自己实现，我们可以把reload也封装起来，保证realod的机制合理：buckup buffer无其他使用者的时候，可以主动调用成功reload
```
file: double_data.h
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
       if(is_allow_reload()) {
           _pre_ptr->reload(std::forward<Args>(args)...);
       }
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
```

## 改进
上面的版本已经是个基本可用的版本，存在这几个问题：  
- 需要数据实现reload，buffer的类型需要和reload耦合，显然操作buffer的reload可以拆出来成为reloder，同样的数据可以配置不同的relaoder，实现解耦  
- 暴露给业务的接口太抽象，业务需要自己去实现各种热加载、主动更新，文件监听、更新机制 

我们可以考虑实际工作中最需要的场景：  
- 读取配置/词典，业务只需要定义自己的Loader和DataType
- 可配置的定期监听配置文件/词典，可选是否自动监听文件变化reload、reload周期，实现热加载
- 手动触发reload  

这样我们可以暴露给用户最简单的接口：
DoubleBuffer<DataType, Loader>
- init //初始化
- get_data //获取数据，这个数据可能会自动更新，但是对用户透明
- reload //主动触发热加载

