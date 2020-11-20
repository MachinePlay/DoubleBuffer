DoubleBuffer
===========
![GitHub](https://img.shields.io/github/license/machineplay/DoubleBuffer)
[![](https://github.com/MachinePlay/DoubleBuffer/workflows/test/badge.svg)](https://github.com/MachinePlay/DoubleBuffer/actions)  

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
是一个HeaderOnly的库，可以直接使用yaml配置版本，也可以继承后实现自己的数据relaod

