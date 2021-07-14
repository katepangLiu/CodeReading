# Apache 的 MPM 类型

Apache 2.x 主要支持三种MPM

- prefork
  - *multi-process*
  - Since 2.0
- worker 
  - *hybrid multi-process multi-threaded*
  - *dedicate a thread to a HTTP connection*
  - Since 2.2
- event 
  - *hybrid multi-process multi-threaded*
  - *dedicate a thread to a request*
  - Since 2.4

其他MPM:

- mpm_winnt  
  - *This Multi-Processing Module is optimized for Windows NT.*
- mpm_netware 
  -  *Multi-Processing Module implementing an exclusively threaded web server optimized for Novell NetWare*
- mpm_os2  
  - *Hybrid multi-process, multi-threaded MPM for OS/2*



## Prefork MPM

在Apache启动时，预先 fork 若干数量的子进程。 

- 每个子进程一次只处理一个请求，提供最佳的隔离性。
- 不需要考虑线程安全的问题

### how it works

唯一的master进程，负责启动多个子进程，接收HTTP请求。
Apache总是尽量维持固定数量的空闲进程，在还没有接收请求之前就做好准备。
这样的好处是，发送请求的Client不需要等待进程fork的时间，请求可以马上被处理。
可以通过配置来指定空闲进程数，默认配置通常只能处理较低的并发，需要按需配置。

### Directives

- StartServers  
- MinSpareServers  
- MaxSpareServers 
- ServerLimit 
- MaxClients 
- MaxRequestsPerChild 

```shell
<IfModule prefork.c>
StartServers       8
MinSpareServers    5
MaxSpareServers   20
ServerLimit      256
MaxClients       256
MaxRequestsPerChild  4000
</IfModule>
```



## Worker MPM

用子进程的子线程来处理请求，可以处理更大的并发数，提高系统资源的使用效率
优点：内存使用和性能比 prefork好。
缺点：没法与php之类的语言一起使用

### how it works

master进程负责启子进程，每个子进程创建固定数量的Server线程，线程数由ThreadPerChild指定;
一个Listener线程监听端口连接，并将连接传递给一个Server进程。

Apache 尽量保证有一组空闲的Server线程，为到来的请求提供服务。
这种的好处是，Client的请求可以直接被处理，不需要等待新线程/新进程的创建。
在运行过程中，Apache 会评估所有进程中空闲线程的总数，并fork或kill进程以将该数量保持在 MinSpareThreads 和 MaxSpareThreads 的限制内。
这个过程是高度自治的，所以一般不需要修改这些参数。

### Directives

- StartServers
- MaxClient
- MinSpareThreads
- MaxSpareThreads
- ThreadsPerChild
- MaxRequestsPerChild

```shell
<IfModule worker.c>
StartServers         4
MaxClients         300
MinSpareThreads     25
MaxSpareThreads     75
ThreadsPerChild     25
MaxRequestsPerChild  0
</IfModule>
```



## Event MPM

为得到更大的并发处理能力，主线程将一些处理工作传递给支持线程，以便主线程可以处理新的请求。
Event MPM的处理方式与worker类似，最大的差别在于，Event MPM 将线程分配给一个请求，而不是分配给一个HTTP连接。

### how it works 

Event MPM 尝试解决HTTP的 keep alive 问题。当一个Client完成首次请求后，可以保持连接不断开，使用相同的socket发送后续的请求。这种方式可以减少TCP连接带来的开销。
但是，此前的Apache，需要独占一整个子进程/线程，来等待客户端继续发送请求。这种方式显然很浪费资源。
为了解决这个问题，Event MPM 

- 用一个专门的线程来处理所有的监听socket，所有的监听socket都是 Keep Alive状态，
- 而那些通信socket,那些已经执行完handler和protocol filter，剩下要做的事情就是将数据发送给Client。
  - mod_status 的status页面会显示有多少连接处于上述状态。

如果你想用多线程，且有很多长连接的客户端，Event是个不错的选择。

- 使用 Worker MPM，线程将绑定到连接，并且无论请求是否正在处理都保持绑定状态。

- 使用 Event MPM，连接线程仅用于单个请求，并在请求完成后立即释放，
  - 不需要管 HTTP 连接是否仍保持连接，连接由父进程来处理。
  - 由于线程在请求完成后立即释放，它可以用于其他请求。

### Directives

与 worker mpm相同。

```shell
<IfModule event.c>
MinSpareThreads 64
MaxSpareThreads 128
ThreadsPerChild 64
ThreadLimit 64
MaxRequestsPerChild 20000
ListenBacklog 4096
</IfModule>
```







