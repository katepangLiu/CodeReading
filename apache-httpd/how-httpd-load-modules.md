# httpd 如何加载模块

## Overview

### 模块类型

Apache httpd server 是一个模块化程序，支持2种形式的模块

- DSO (Dynamic Shared Object)
  - built-time
  - runtime
- static module

httpd源码中，static modules 用一个数组定义:

```c
module *ap_prelinked_modules[] = {
  &core_module, /* core must come first */
  &mpm_netware_module,
  &http_module,
  &so_module,
  &mime_module,
  &authn_core_module,
  &authz_core_module,
  &authz_host_module,
  &negotiation_module,
  &include_module,
  &dir_module,
  &alias_module,
  &env_module,
  &log_config_module,
  &setenvif_module,
  &watchdog_module,
#ifdef USE_WINSOCK
  &nwssl_module,
#endif
  &netware_module,
  NULL
};
```

dynamic modules 由 mod_so 模块 负责加载。

### DSO的加载

DSO的加载主要依赖于 mod_so 模块，mod_so 必须以静态形式编译进 httpd core。

**加载步骤**：

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
      - `ap_add_module(module *m, apr_pool_t *p, const char *sym_name)`
        - put into map, and get index
        - `ap_add_module_commands(m, p);`
        - `ap_register_hooks(m, p);`
    - 注册module实例 清理函数，进程关闭时清理实例。

```c
/*
 * This is called for the directive LoadModule and actually loads
 * a shared object file into the address space of the server process.
 */

static const char *load_module(cmd_parms *cmd, void *dummy,
                               const char *modname, const char *filename)
{
    apr_dso_handle_t *modhandle;
    apr_dso_handle_sym_t modsym;
    module *modp;
    const char *module_file;
    so_server_conf *sconf;
    ap_module_symbol_t *modi;
    ap_module_symbol_t *modie;
    int i;
    const char *error;

    /* we need to setup this value for dummy to make sure that we don't try
     * to add a non-existent tree into the build when we return to
     * execute_now.
     */
    *(ap_directive_t **)dummy = NULL;

    /*
     * check for already existing module
     * If it already exists, we have nothing to do
     * Check both dynamically-loaded modules and statically-linked modules.
     */
    sconf = (so_server_conf *)ap_get_module_config(cmd->server->module_config,
                                                &so_module);
    modie = (ap_module_symbol_t *)sconf->loaded_modules->elts;
    for (i = 0; i < sconf->loaded_modules->nelts; i++) {
        modi = &modie[i];
        if (modi->name != NULL && strcmp(modi->name, modname) == 0) {
            ap_log_perror(APLOG_MARK, APLOG_WARNING, 0, cmd->pool, APLOGNO(01574)
                          "module %s is already loaded, skipping",
                          modname);
            return NULL;
        }
    }

    for (i = 0; ap_preloaded_modules[i]; i++) {
        const char *preload_name;
        apr_size_t preload_len;
        apr_size_t thismod_len;

        modp = ap_preloaded_modules[i];

        /* make sure we're comparing apples with apples
         * make sure name of preloaded module is mod_FOO.c
         * make sure name of structure being loaded is FOO_module
         */

        if (memcmp(modp->name, "mod_", 4)) {
            continue;
        }

        preload_name = modp->name + strlen("mod_");
        preload_len = strlen(preload_name) - 2;

        if (strlen(modname) <= strlen("_module")) {
            continue;
        }
        thismod_len = strlen(modname) - strlen("_module");
        if (strcmp(modname + thismod_len, "_module")) {
            continue;
        }

        if (thismod_len != preload_len) {
            continue;
        }

        if (!memcmp(modname, preload_name, preload_len)) {
            return apr_pstrcat(cmd->pool, "module ", modname,
                               " is built-in and can't be loaded",
                               NULL);
        }
    }

    modi = apr_array_push(sconf->loaded_modules);
    modi->name = modname;

    /*
     * Load the file into the Apache address space
     */
    error = dso_load(cmd, &modhandle, filename, &module_file);
    if (error)
        return error;
    ap_log_perror(APLOG_MARK, APLOG_DEBUG, 0, cmd->pool, APLOGNO(01575)
                 "loaded module %s from %s", modname, module_file);

    /*
     * Retrieve the pointer to the module structure through the module name:
     * First with the hidden variant (prefix `AP_') and then with the plain
     * symbol name.
     */
    if (apr_dso_sym(&modsym, modhandle, modname) != APR_SUCCESS) {
        char my_error[256];

        return apr_pstrcat(cmd->pool, "Can't locate API module structure `",
                          modname, "' in file ", module_file, ": ",
                          apr_dso_error(modhandle, my_error, sizeof(my_error)),
                          NULL);
    }
    modp = (module*) modsym;
    modp->dynamic_load_handle = (apr_dso_handle_t *)modhandle;
    modi->modp = modp;

    /*
     * Make sure the found module structure is really a module structure
     *
     */
    if (modp->magic != MODULE_MAGIC_COOKIE) {
        return apr_psprintf(cmd->pool, "API module structure '%s' in file %s "
                            "is garbled - expected signature %08lx but saw "
                            "%08lx - perhaps this is not an Apache module DSO, "
                            "or was compiled for a different Apache version?",
                            modname, module_file,
                            MODULE_MAGIC_COOKIE, modp->magic);
    }

    /*
     * Add this module to the Apache core structures
     */
    error = ap_add_loaded_module(modp, cmd->pool, modname);
    if (error) {
        return error;
    }

    /*
     * Register a cleanup in the config apr_pool_t (normally pconf). When
     * we do a restart (or shutdown) this cleanup will cause the
     * shared object to be unloaded.
     */
    apr_pool_cleanup_register(cmd->pool, modi, unload_module, apr_pool_cleanup_null);

    /*
     * Finally we need to run the configuration process for the module
     */
    ap_single_module_configure(cmd->pool, cmd->server, modp);

    return NULL;
}
```





## module 数据结构

- API版本兼容性
  - version
  - minor_version
- index   当前模块在模块数组中的索引
- next    下一个模块
- name  模块文件名
- dynamic_load_handle   模块文件句柄
- config
  - dir_config   (per directory configuration)
  - server_config  (the per server configuration)
- cmds   
  - 模块支持的指令集
- register_hooks
  - hook 注册函数
  - 在加载时被调用  `ap_register_hooks`
- flags



```c
/**
 * Module structures.  Just about everything is dispatched through
 * these, directly or indirectly (through the command and handler
 * tables).
 */
typedef struct module_struct module;
struct module_struct {
    /** API version, *not* module version; check that module is
     * compatible with this version of the server.
     * API 版本，用于校验模块的兼容性，不是模块的版本号
     */
    int version;
    /** API minor version. Provides API feature milestones. Not checked
     *  during module init */
    int minor_version;
    /** Index to this modules structures in config vectors.  */
    int module_index;

    /** The name of the module's C file */
    const char *name;
    /** The handle for the DSO.  Internal use only */
    void *dynamic_load_handle;

    /** A pointer to the next module in the list
     *  @var module_struct *next
     */
    struct module_struct *next;

    /** Magic Cookie to identify a module structure;  It's mainly
     *  important for the DSO facility (see also mod_so).  */
    unsigned long magic;

    /** Function to allow MPMs to re-write command line arguments.  This
     *  hook is only available to MPMs.
     *  @param The process that the server is running in.
     */
    void (*rewrite_args) (process_rec *process);
    /** Function to allow all modules to create per directory configuration
     *  structures.
     *  @param p The pool to use for all allocations.
     *  @param dir The directory currently being processed.
     *  @return The per-directory structure created
     */
    void *(*create_dir_config) (apr_pool_t *p, char *dir);
    /** Function to allow all modules to merge the per directory configuration
     *  structures for two directories.
     *  @param p The pool to use for all allocations.
     *  @param base_conf The directory structure created for the parent directory.
     *  @param new_conf The directory structure currently being processed.
     *  @return The new per-directory structure created
     */
    void *(*merge_dir_config) (apr_pool_t *p, void *base_conf, void *new_conf);
    /** Function to allow all modules to create per server configuration
     *  structures.
     *  @param p The pool to use for all allocations.
     *  @param s The server currently being processed.
     *  @return The per-server structure created
     */
    void *(*create_server_config) (apr_pool_t *p, server_rec *s);
    /** Function to allow all modules to merge the per server configuration
     *  structures for two servers.
     *  @param p The pool to use for all allocations.
     *  @param base_conf The directory structure created for the parent directory.
     *  @param new_conf The directory structure currently being processed.
     *  @return The new per-directory structure created
     */
    void *(*merge_server_config) (apr_pool_t *p, void *base_conf,
                                  void *new_conf);

    /** A command_rec table that describes all of the directives this module
     * defines. */
    const command_rec *cmds;

    /** A hook to allow modules to hook other points in the request processing.
     *  In this function, modules should call the ap_hook_*() functions to
     *  register an interest in a specific step in processing the current
     *  request.
     *  @param p the pool to use for all allocations
     */
    void (*register_hooks) (apr_pool_t *p);

    /** A bitmask of AP_MODULE_FLAG_* */
    int flags;
};
```





## 查看所有已加载的模块

- httpd -l    preloaded module
- httpd -L   查看所有 loaded_module 及其支持的指令



```shell
[root@pang httpd]# httpd -l
Compiled in modules:
  core.c
  mod_so.c
  http_core.c
```



```shell
[root@pang httpd]# httpd -L
...
exampleEnabled (mod_example_config_simple.c)
	Enable or disable mod_example
	Allowed in *.conf only outside <Directory>, <Files>, <Location>, or <If>
examplePath (mod_example_config_simple.c)
	The path to whatever
	Allowed in *.conf only outside <Directory>, <Files>, <Location>, or <If>
exampleAction (mod_example_config_simple.c)
	Special action value!
	Allowed in *.conf only outside <Directory>, <Files>, <Location>, or <If>
```



## Reference

https://httpd.apache.org/docs/2.4/dso.html

https://httpd.apache.org/docs/2.4/mod/mod_so.html

