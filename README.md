# :mega: PulseFlow 脉冲流 :ghost: :waxing_crescent_moon:

# 背景描述
随着公司PHP项目体的不断增大，随着不同工程师的功能迭代，如何有效获取PHP项目的执行性能，对于系统整体模块显得异常重要，PulseFlow是一个性能跟踪扩展，它可以在程序员无感知的情况下有效跟踪每一个函数的执行效率，主要分析CPU时间消耗、内存大小消耗，这个组件除了能够快速记录每个函数体的性能信息，还具备一系列的发送机制，主要包括共享内存队列（System V 和 Posix）、UDP、Unix Domain Socket 等模式。

# 编译安装 :heavy_check_mark:
此为PHP扩展，按照正常的扩展安装，我们需要执行以下几步操作：

1. git clone https://github.com/gitsrc/PulseFlow.git
2. 进入源码目录
3. phpize
4. ./configure --with-php-config= （ php-config文件路径） CFLAGS='-g -lrt'  CXXFLAGS='-lrt'
5. make
6. make install

## 编译特别注意 :heavy_check_mark:
由于在扩展中使用了 posix 共享内存队列，所以我们编译时需要引入lrt库，上面在configure过程中附加的 CFLAGS 和 CXXFLAGS 参数用来给Makefile中的相应字段添加 -lrt，最终构造完后的Makefile对应配置应该为：

```shell
CFLAGS = -g -lrt -O0

CXXFLAGS = -lrt -g -O0

```

# 设计点1：PHP INI 配置选项

PulseFlow由于是一个基于C语言的PHP扩展，为了保持程序体的扩展性，配置选项一律从php.ini文件中读取，本节将描述所有与扩展程序有关系的配置信息。

## 1.1 扩展功能开关参数 （PulseFlow.enabled） :heavy_check_mark:

### 1.1.1 参数介绍
这个参数是插件的功能开关，属于布尔类型，有效参数如下，默认 false：

1.  true：开启

2.  false：关闭

## 1.2 日志功能开关参数 （PulseFlow.debug）

### 1.2.1 参数介绍
这个参数是控制插件是否向页面输出调试信息，有效参数如下，默认 false：

1. true：开启

2. false：关闭

## 1.3 禁止跟踪函数列表 （PulseFlow.disable_trace_functions） :heavy_check_mark:

### 1.3.1 参数介绍
这个参数是一个逗号分隔的字符串，代表一系列函数列表，这个函数列表内的函数不进行性能跟踪，默认空字符串。

配置样例：PulseFlow.disable_trace_functions = "getLoader,findFile,loadClassLoader,getInitializer,findFileWithExtension,"

## 1.4 禁止跟踪类列表 （PulseFlow.disable_trace_class）:heavy_check_mark:

### 1.4.1 参数介绍
这个参数是一个逗号分隔的字符串，代表一系列类列表，这个类列表内的类不进行性能跟踪，默认空字符串。

配置样例：PulseFlow.disable_trace_functions = "class1,class2,class3"

## 1.5 跟踪模式 （PulseFlow.trace_mode）

### 1.5.1 参数介绍
这个参数主要用来设置插件的性能跟踪工作模式，目前分类三类：simple、class、function 默认simple

1. simple：简单模式、代表每个用户执行流程都将进行跟踪，排除在php.ini中配置的函数列表 及 类列表

2. class：类模式、根据类进行作为采样率

3. function：函数模式、根据函数作为采样率

配置样例： PulseFLow.trace_mode = "simple"

## 1.6 跟踪采样率 （PulseFlow.trace_rate）

### 1.6.1 参数介绍

这个参数主要用来设置采样频率，默认值为0 ，代表每次都采集。 如果参数不为0 ,假设为n，则代表每n次进行一次采集。


# 设计点2：保存方式 
数据保存方式，由于监控获取的数据集，如何更快更好的存储起来，也是保持链路高效的元素之一。

## 2.1 保存介质（目前实现内存堆区）:heavy_check_mark:

数据保存，首先应该考虑数据存储介质，好的存储介质为高速存储提供基础保障，目前主要考虑以下三种方案。

### 2.1.1 内存堆区

目前扩展基于堆区存储监控数据，没有发现内存泄露情况，后期会参考linux内核文件表结构，加入栈结构缓冲区来优化性能。

### 2.1.2 内存栈区

后期会借助内存栈区来实现数据缓冲区。

### 2.1.3 寄存器区

借助编译器，目前部分存储已经被优化至寄存器区。

## 2.2 数据编码（ PulseFlow.encode_type）:heavy_check_mark:

数据除了要注意存储介质以外，还需要注意数据编码格式、目前提供了Json序列化方式。

### 2.2.1 配置参数 
用户可以在 php.ini 通过修改 PulseFlow.encode_type 的值来进行编码切换，默认值是json，目前提供的编码方式如下：

1. json 

# 设计点3：发送模式（ PulseFlow.send_type）

数据完整保存后，如何保障数据能够高速稳定的发送出去，是PulseFlow的最后关键一环。

## 3.1发送通路介质选择

首先从发送通路介质考虑，可以考虑如下四种方案，共享内存队列 、UDP、Unix domain Socket、TCP，目前为了保证最快的传输速度，我们选择了共享内存队列这种方案，在 Linux 环境下、目前提供了 System V 和 Posix 两大共享内存队列。

### 3.1.1 共享内存队列 :heavy_check_mark:

目前实现了 System V 和 Posix 两大共享内存队列，两个队列的优缺点可以网络上自我查询，对于Posix 队列的服务端程序，我们采用了EPOLL 复用模型 来进行 I/O 监控。

#### 共享队列相关配置参数

1. **PulseFlow.send_type** ：数据发送类型，目前有posix 和 svipc ，**默认为 pos**ix

2. 如果选择了 **posix**，则可以配置 **PulseFlow.posix_name** 参数，默认为 “**/PulseFlow_posix_ipc**”

3. 如果选择了 **svipc**，则可以配置 （ **PulseFlow.svipc_name**参数 ：默认值为“**/dev/shm/PulseFlow_sv_ipc**”） 和 （ **PulseFlow.svipc_pj_id**参数 ： 默认为 **1000**）

#### **运行环境特别注意**：

1. **由于 svipc 的默认文件路径为 “/dev/shm/PulseFlow_sv_ipc” ，所以务必保证此文件实际存在，并且 php-fpm 有 写入权限。**

2. **由于 posix 的原理是给予内存映射文件，所以请务必保证 程序对于 /dev/mqueue/ 目录有读写权限。**



### 3.1.2 UDP

### 3.1.3 Unix domain socket

### 3.1.4 TCP


# 设计点4：扩展函数

##  4.1 pulseflow_enable 函数 :heavy_check_mark:
启用 Pulse_FLow 监听功能，这个函数是在给扩展打开性能监控开关，这个函数之后的代码可以被监控起来。

## 4.2 pulseflow_disable 函数 :heavy_check_mark:
禁用 Pulse_FLow 监听功能，这个函数是在给扩展关闭性能监控开关，这个函数之后的代码失去监控状态。


