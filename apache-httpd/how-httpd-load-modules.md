# httpd 如何加载模块

## Overview

- 进程启动时，加载 prelinked_modules
- prelinked_modules 中的 [mod_so](https://httpd.apache.org/docs/2.4/mod/mod_so.html), 支持 指令  `LoadModule` 和 `LoadFile`
  - `LoadModule module_name module_path`  指定要加载的模块名和模块文件
  -  `LoadFile libpath `  指定需要加载的库文件
- LoadModule 指令对应的函数 load_module()
  - 判断模块是否已加载，避免重复加载
  - 检查模块名是否满足格式，且不能与内建模块同名
  - 插入模块列表 `sconf->loaded_modules`
  - 加载模块
    - 模块文件加载到地址空间
    - 定位到module数据结构的地址 `modp`
    - 校验 魔数，判断是否是 module 数据结构
    - 添加 modp 到 core 数据结构中  `ap_add_loaded_module(modp, cmd->pool, modname)`
    - 注册module实例 清理函数，进程关闭时清理实例。

