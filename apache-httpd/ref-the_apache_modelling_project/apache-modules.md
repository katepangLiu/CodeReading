# Apache Modules

## 模块的基本要素

![image-20210715173017537](apache-modules.assets\image-20210715173017537.png)

## 交互模型

![image-20210715173032409](apache-modules.assets\image-20210715173032409.png)



- Server core  通过module注册的信息, 调用其 handler
- modules 通过 Server core 暴露的接口，访问关键的数据结构，完成业务逻辑

## Handlers

module可以提供多种形式的handler：

- handlers for hook
- handlers for dealing with configuration directives
- filters
- Optional functions

### handlers for hook

调用方通过执行 ap_run_HookName 调用handlers
调用方式有2种：

- RUN_ALL/VOID  依次执行所有的handler；直到发生错误
- RUN_FIRST          依次执行，只要有handler一个处理完成，后续的不再执行

**任何模块都可以定义 hook 供其他模块注册handler**

![image-20210715175407155](apache-modules.assets\image-20210715175407155.png)

### handlers for Config Directives

定义模块到某一个配置指令的处理函数



### Optional functions

> Since Apache 2.0

机制与 hooks 类似 (opt func 一般用以执行一些非必要的处理)

- core 会忽略 optional function 的返回值

- core 要处理 hook handler 的返回值

### Filters

#### Content Handling

接收并响应HTTP请求的过程中，最重要的环节是调用 content handler,  它负责发送响应给Client。

##### Apache 1.3 Content Handling

**内容处理流程**

- type_checker handler
  - 将请求的资源  映射 到 对应的 mime-type，及其 handler
- 执行 相应的 handler 来处理请求
  - 可以直接发送响应到网络接口

**分析**

- 优点
  - 简单直接
- 缺点
  - 只有一个模块可以处理请求
  - 没法灵活地变更响应逻辑，需要修改代码

##### Apache 2.0  Content Handling

- 使用 Output Filters 来 拓展 内容处理机制
- 仍然只有一个 content handler 被调用，来发送响应
- Filters 可以 修改 content handler 的输出

分析

- 优点
  - 多个模块可以参与处理请求
  - 每个 mime-type 可以依靠
    - a set of modules
    - a output filter chain

#### Filters

>Since 2.0

Filters are handlers for processing data of the request and reponse.

- Input filter chain for request data.
- Output filter chain for reponse data 
  - data provided by content handler.
  - triggered by content handler.

mod_ssl 就是使用 filter chain 来提供 HTTPS 通信 :

![image-20210715190627480](apache-modules.assets\image-20210715190627480.png)

##### Performance

A Brigade contains a series of buckets:

![image-20210715192529368](apache-modules.assets\image-20210715192529368.png)

A Filter chain using Brigades for data transport:

![image-20210715192630779](apache-modules.assets\image-20210715192630779.png)

Filter 可以分为三类：

-  Resource and Content Set Filters
  - alter the content that is passed through them.
- Protocol and Transcode Filters
  -  implement the protocol’s behavior (HTTP, POP, . . . ).
- Connection and Network Filters
  - deal with establishing and releasing a connection



### Predefined Hooks

1. Managing and processing confifiguration directives

2. Start–up, restart or shutdown of the server

3. Processing HTTP requests

#### 1. Configure Management Hooks

- create per-directory config
- merge per-directory config
- create per-server config
- merge per-server config
- cmd handles

#### 2. Start–up, restart or shutdown of the server

- pre_config
- open_logs
- post_config
- pre_mpm (internal)
- child_init
- child_exit (Apache 1.3 only)

#### 3. Receiving and Processing HTTP requests

##### Establish a connection and read the request

![image-20210715194601339](apache-modules.assets\image-20210715194601339.png)

- create_connection
- pre_connection
- process_connection
- create_request (internal)
- post_read_request



##### Process a request and send a response

![image-20210715194812018](apache-modules.assets\image-20210715194812018.png)

- quick_handler

- translate_name

- map_to_storage (internal)

- header_parser

- access_checker

- check_user_id

- auth_checker

- type_checker

- fixups

  - At this step all modules can have a last chance to modify the response header

    (for example set a cookie) before the calling the content handler

- insert_filter

  - This hook lets modules insert filters in the output filter chain.

- handler

  - The hook for the content handler is the most important one in the request response loop: 
    - generates or reads the requested resource 
    - and sends data of the response to the client using the output filter chain.

- log_translation

  - Here each module gets a chance to log its messages after processing the request.



## The Apache API

前面的handlers 讲的是如何调用modules执行任务，API 则是基础库，支撑modules完成实际的任务。
Apache提供了一系列供modules调用的接口，
这些接口的输入和输出，很多是Apache定义好的数据结构。
2.0 引入了 Apache Portable Runtime 大大增强了API的功能。

### Memory management with Pools



### Data types

- request_rec
- server_rec
- connection_rec



### API functions

- Memory Management
- Multitasking (Processes only)
- Array Manipulation
- Table Manipulation
- String Manipulation (e.g. parsing, URI, Cookies)
- Network I/O (Client Communication only)
- Dynamic Linking
- Logging
- Mutex for fifiles
- Identifification, Authorization, Authentication



### Apache Portable Runtime (APR)

**features**:

- File I/O + Pipes
- Enhanced Memory Management
- Mutex + Locks, Asynchronous Signals
- Network I/O
- Multitasking (Threads & Processes)
- Dynamic Linking (DSO)
- Time
- Authentication

A goal ot the APR is to set up a common framework for network servers. Any request processing server could be implemented using the Apache core and the APR.

