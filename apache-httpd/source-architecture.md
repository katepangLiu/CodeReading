# Apache源码结构

## Overview

- 可移植运行库 (APR)
- 核心功能层 (Apache Core)
- 可选模块 (Apache Optional Module)
- 第三方支持库

![image-20210713144441417](source-architecture.assets\image-20210713144441417.png)



### APR

屏蔽底层的操作系统API细节，提供通用的API接口

APR 现在是一个独立的项目，不仅限于为 httpd 提供支撑

![image-20210713145155343](source-architecture.assets\image-20210713145155343.png)

### Apache Core

核心层包含2大部分，Apache Core 和 Core modules

- Apache Core

  - 实现Apache作为HTTP服务器的基本功能
    - 启停 Apache
    - 处理配置文件
    - 接收和处理HTTP连接
    - 读取HTTP请求并处理
    - 处理HTTP协议

- Apache Core modules

  - 必需的模块
    - mod_core
      - 处理配置文件中的大部分配置指令
      - 根据这些指令允许Apache
    - mod_so
      - 加载其余的模块

  

  Apache Core 为 modules 提供一系列的API, 这里使用的是 事件驱动的模型。
  每个模块可以注册各种回调函数，Core在处理请求时，触发相应的回调。
  每个请求被划分成多个不同的阶段。模块想要参与哪些阶段的处理，则注册相应的hook；每个回调函数，返回 OK/DECLINE，向Core反馈处理情况。

  

### Apache Optional Module

mod_so 根据 `LoadModule` 指令，加载指定的模块。

核心模块与非核心模块的唯一区别是，加载时间不同。



## Apache Core

核心组件主要包括以下几个部分

- 配置文件组件 (CONFIG)  (http_config.c, config.c)
- 进程并发组件 (MPM)
- HTTP连接处理组件 (HTTP_CONNECTION) (connection.c)
- HTTP请求处理组件 (HTTP_REQUEST) (request.c)
- HTTP核心组件 (HTTP_CORE) (http_core.c)
- 核心模块组件  (MOD_CORE) (core.c)

![image-20210714105923294](source-architecture.assets\image-20210714105923294.png)

### main

- 命令行处理
- 配置文件处理
- 构建虚拟主机信息
  - 从配置信息中获取虚拟主机配置信息
  - 构建虚拟主机
  - 保存成一系列的 `server_rec` 链表
- 进入主循环
  - 执行 MPM模块
  - 产生多个进程/线程
  - 失败后重启 MPM

### HTTP 配置文件组件（HTTP_CONFIG）

- 处理配置信息，整理成 配置树 (N叉树)
- 提供 配置 访问接口

### 进程并发处理组件（MPM）

主要源文件：

- mpm/xxx.c  (如 prefork.c)



分析：

- 提供多种方式的并发处理
- 只能加载一个 MPM，选定一种并发方式
- 必须在编译时指定，不允许动态加载

### HTTP连接处理组件（HTTP_CONNECTION）

主要源文件：

- http_connection.h
- connection.c

### HTTP协议处理组件（HTTP_PROTOCOL）

主要源文件：

- http_protocol.h
- http_protocol.c

分析：

- 主要负责处理 HTTP/1.0 HTTP1.1 协议的解析
  - 解析请求头
  - 生成响应包

### 请求处理组件HTTP_REQUEST

主要源文件

- request.c
- http_request.h
- http_request.c

分析：

- **请求的总入口函数 ap_process_request 及其终止函数**
  - 进入实质的请求处理阶段
  - 请求失败时，调用 ap_die 终止
- **对请求属性的操作**
  - 提供一系列的函数，对请求的属性进行更改
    - ap_update_mtime
    - ap_allow_methods
- **子请求相关操作**
  - 派生子请求，并发处理
    - mod_autoindex 针对目录中的每个文件产生一个子请求
    - 子请求完成后必须返回父请求
  - 相关函数
    - ap_sub_req_loopup_rui
    - ap_sub_req_loopup_file
    - ap_sub_req_loopup_dirent
    - ap_sub_req_method_uri
    - ap_run_sub_req
    - ap_destroy_sub_req
- **重定向请求操作**
  - 请求有多种情况可能发生重定向
    - 请求处理发生错误
    - 请求被rewrite
  - 重定向发生时，产生一个新的请求，由心情去继续处理
  - 重定向处理结束后不一定会返回原来的请求
  - 相关函数
    - ap_internal_redirect
    - ap_internal_redirect_handler
    - ap_internal_fast_redirect
- **通知其他模块，当前处于什么阶段, 触发 hook**
  - hooks
    - create_request
    - translate_name
    - map_to_storage
    - check_user_id
    - ...
- **请求相关的配置处理**
  - 查找与该请求相关联的配置信息



### HTTP核心组件（HTTP_CORE）

源文件：

- http_core.h
- http_core.c

分析：

- 以一个动态连接模块的形式提供（ap_prelinked_modules 中）

- 处理HTTP协议相关的指令
- 从 core.c 中剥离出来，保证 core 的 协议无关性

### MOD_CORE

源文件：

- mod_core.h
- core.c

分析：

- 处理核心指令
  - Directory
  - Location
  - DocumentRoot
- 在 HTTP_CONFIG 中被调用

### 其余模块

- 日志处理组件
- 虚拟主机处理组件
- 过滤器模块



## Apache 运行流程

![apache-startup](source-architecture.assets\apache-startup.png)

- A部分： Apache 启动过程
- B部分：接受客户端连接，处理该连接
- C部分：从连接中读取请求数据，处理一次客户端请求



### Apache 的两阶段启动

- 高权限启动(root)
  - 资源初始化
    - 内存池
    - 配置读取（2次）
      - 预读取： 为真正的配置文件读取做准备
      - 真正的配置文件读取
    - 虚拟主机
    - 数据库
  - ap_mpm_run,  MPM得到控制权
    - 使用低权限启动若干个worker (进程/线程)
    - 不同的MPM使用不同的并发模型，适用于不同的操作系统
- 低权限启动 worker (默认时apache用户，降低恶意攻击的危害)
  - 监听指定的端口，等待客户端连接
  - 接收客户端连接，进入连接处理和请求处理阶段

### HTTP 连接处理

- 获取IP地址，确定对应的 虚拟主机
- 触发 process_connection hooks
- ap_read_request 
- ap_process_request

### 请求读取

- HTTP_PROTOCOL 进行HTTP报文解析
- 进入 Input Filter 进行处理
- 交给 HTTP_REQUEST

### 请求处理

ap_process_request 时 请求处理的入口

- 请求解析
  - URL 处理
    - translate_name
    - map_to_storage
  - header_parser，
    - 例如，根据请求头来设置内部的环境变量
- 认证&鉴权
  - access_checker  粗粒度的认证
    - IP
    - ...
  - check_user_id    用户名&密码
  - auth_checker    具体资源的访问权限
- 请求准备
  - type_checker
    - mod_mime 确认客户请求的资源类型
    - fixups 在进程 内容生成之前，对请求做一些 修补工作
- 内容生成
  - 静态
    - 读取HTML ，发送响应
  - 动态脚本  (请求后端的内容生成器，生成响应)
    - CGI
    - ...
- Output Filter 处理
  - 依次流过 Filter Chain 中的Filter 
  - 最后一个Filter 是 网络输出过滤器，负责网络传输



### 关闭与重启

2种方式，发送unix信号 和 apachectl

#### 发送unix 信号

应该向 master进程发送信号，由master管理 childs

- TERM  立即停止
- HUP    重启
- USR1  优雅停止

#### apachectl

- apachectl stop 立即停止
- apachectl restart 重启
- apachectl graceful-stop 优雅停止

## mian()

![image-20210714150728397](source-architecture.assets\image-20210714150728397.png)

# Reference

[Apache源代码全景分析第1卷](https://book.douban.com/subject/3683326/)

