# Apache Configuring

Apache 的配置大体可以分三部分：

- Command-line parameters
- Global Config  (per-server conf)
  - 默认`httpd.conf`
    - 可以通过命令行参数指定其他的配置文件
  - 在Server启动时，处理配置
- Local Config   (per-direction conf)
  - 默认`.htaccess`   
    - 可以通过 httpd.conf 中的 AccessFileName 指定其他文件
  - 在HTTP请求相关目录时，处理配置

![image-20210715160632288](configuring.assets\image-20210715160632288.png)

## Global Config

配置项大体可以分三类

- 控制 全局
- 控制 Master Server
- 虚拟主机的相关设置

### 配置的作用域

如果你希望做局部的设置，可以限定 配置的 的作用域

- Directory   [文件的真实目录]
  - <Diectory>
  - <DirectoryMatch>
- Files
  - <Files>
  - <FilesMatch>
- Location   [URI]
  - <Location>
  - <LocationMatch>

### 配置的条件控制

- [if](https://httpd.apache.org/docs/2.4/mod/core.html#if)
- [ifdefine](https://httpd.apache.org/docs/2.4/mod/core.html#ifdefine)
- [ifdirective](https://httpd.apache.org/docs/2.4/mod/core.html#ifdirective)
- [iffile](https://httpd.apache.org/docs/2.4/mod/core.html#iffile)
- [ifmodule](https://httpd.apache.org/docs/2.4/mod/core.html#ifmodule)
- [ifsection](https://httpd.apache.org/docs/2.4/mod/core.html#ifsection)

## Local Config

对同目录及其子目录的资源做控制，可以设置5种内容

- AuthConfig
- Limits
- Options
- FileInfo
- Indexes

将这些配置放在 httpd.conf ，然后用 Directory 限制其作用域，和放到目录下的 .htaccess，作用是等效的。

## 如何关联一个请求的配置

- Determine virtual host

- Location walk
- Translate Request URL  (mod_rewrite)
- Directory walk beginning from root (/), applying .htaccess files (directory_walk)
- File walk
-  Location walk again, in case Request URI has been changed

## 示例

*httpd.conf*

```ini
#########################################
# Section 1: Global Environment
# Many of the values are default values, so the directives could be omitted.
ServerType standalone
ServerRoot "/etc/httpd"
Listen 80
Listen 8080
Timeout 300
KeepAlive On
MaxKeepAliveRequests 100
KeepAliveTimeout 15
MinSpareServers 5
MaxSpareServers 10
StartServers 5
MaxClients 150
MaxRequestsPerChild 0
#########################################
# Section 2: "Main" server configuration
ServerAdmin webmaster@foo.org
ServerName www.foo.org
DocumentRoot "/var/www/html"
# a very restrictive default for all directories
<Directory />
    Options FollowSymLinks
    AllowOverride None
</Directory>
    <Directory "/var/www/html">
    Options Indexes FollowSymLinks MultiViews
    AllowOverride None
    Order allow,deny
    Allow from all
</Directory>
#########################################
# Section 3: virtual hosts
<VirtualHost www.foo.dom:80>
    # all hosts in the hpi.uni-potsdam.de domain are allowed access;
    # all other hosts are denied access
    <Directory />
        Order Deny,Allow
        Deny from all
        Allow from hpi.uni-potsdam.de
    </Directory>
    # the "Location" directive will only be processed if the module
    # mod_status is part of the server
    <IfModule mod_status.c>
            <Location /server-status>
            SetHandler server-status
            Order Deny,Allow
            Deny from all
            Allow from .foo.com
        </Location>
    </IfModule>
</VirtualHost>
```





