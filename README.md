# :mega: PulseFlow 脉冲流 :ghost: :waxing_crescent_moon:
PulseFlow是一个PHP扩展，它可以无感知的运行在Zend 内核里，自动拦截并分析每个函数运行时的CPU和内存信息。

基于C进行开发，请使用PHP7.0以上版本
# 背景描述
随着公司PHP项目体的不断增大，随着不同工程师的功能迭代，如何有效获取PHP项目的执行性能，对于系统整体模块显得异常重要，PulseFlow是一个性能跟踪扩展，它可以在程序员无感知的情况下有效跟踪每一个函数的执行效率，主要分析CPU时间消耗、内存大小消耗，这个组件除了能够快速记录每个函数体的性能信息，还具备一系列的发送机制，主要包括UDP、Unix Domain Socket 等模式。

# 设计点1：配置选项 
PulseFlow由于是一个基于C语言的PHP扩展，为了保持程序体的扩展性，配置选项一律从php.ini文件中读取，本节将描述所有与扩展程序有关系的配置信息。

## 1.1 扩展功能开关参数 （PulseFlow.enabled） :heavy_check_mark:

### 1.1.1 参数介绍
这个参数是插件的功能开关，属于布尔类型，有效参数如下，默认关闭：

1.  true：开启
2.  false：关闭

## 1.2 日志功能开关参数 （PulseFlow.debug） :heavy_check_mark:

### 1.2.1 参数介绍
这个参数是控制插件是否向页面输出调试信息，有效参数如下，默认关闭：

1. true：开启
2. false：关闭

## 1.3 禁止跟踪函数列表 （PulseFLow.disable_trace_functions） :heavy_check_mark:

### 1.3.1 参数介绍
这个参数是一个逗号分隔的字符串，代表一系列函数列表，这个函数列表内的函数不进行性能跟踪，默认空字符串。

配置样例：PulseFlow.disable_trace_functions = "getLoader,findFile,loadClassLoader,getInitializer,findFileWithExtension,"

## 1.4 禁止跟踪类列表 （PulseFLow.disable_trace_class） :heavy_check_mark:

### 1.4.1 参数介绍
这个参数是一个逗号分隔的字符串，代表一系列类列表，这个类列表内的类不进行性能跟踪，默认空字符串。

配置样例：PulseFlow.disable_trace_functions = "class1,class2,class3"

## 1.5 跟踪模式 （PulseFLow.trace_mode）

### 1.5.1 参数介绍
这个参数主要用来设置插件的性能跟踪工作模式，目前分类三类：simple、class、function 默认simple
1. simple：简单模式、代表每个用户执行流程都将进行跟踪，排除在php.ini中配置的函数列表 及 类列表
2. class：类模式、根据类进行作为采样率
3. function：函数模式、根据函数作为采样率

配置样例： PulseFLow.trace_mode = "simple"

## 1.6 跟踪采样率 （PulseFLow.trace_rate）

### 1.6.1 参数介绍
这个参数主要用来设置采样频率，默认值为0 ，代表每次都采集。 如果参数不为0 ,假设为n，则代表每n次进行一次采集。


# 设计点2：保存方式
数据保存方式，由于监控获取的数据集，如何更快更好的存储起来，也是保持链路高效的元素之一。

## 2.1保存介质
数据保存，首先应该考虑数据存储介质，好的存储介质为高速存储提供基础保障，目前主要考虑以下三种方案
### 2.1.1 内存堆区

### 2.1.2 内存栈区

### 2.1.3 寄存器区

# 设计点3：发送模式
数据完整保存后，如何保障数据能够高速稳定的发送出去，是PulseFlow的最后关键一环。

## 3.1发送通路介质选择
首先从发送通路介质考虑，可以考虑如下三种方案，UDP、Unix domain Socket、TCP、共享内存

### 3.1.1 UDP

### 3.1.2 Unix domain socket

### 3.1.3 TCP

## 

