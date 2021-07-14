# httpd.conf

Apache 的默认配置文件，httpd -f <conf> 可以指定加载的配置文件路径

**包含的子配置文件**

- AccessFile `.htaccess`   提供目录级别的控制
- AccessConfig `access.conf`  访问权限控制
- ResourceConfig `srm.conf`    资源映射文件，描述服务器上的MIME类型及其支持
- TypeConfig `mime.types`  标识不同文件对应的MIME类型

