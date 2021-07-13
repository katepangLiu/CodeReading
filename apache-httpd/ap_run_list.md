## ap_run_xxx_list

## main

- ap_run_rewrite_args
- ap_run_open_logs
- ap_run_optional_fn_retrieve
- ap_run_mpm
- config
  - ap_run_pre_config
  - ap_run_check_config
  - ap_run_test_config
  - ap_run_post_config
  - ap_run_pre_config



## config

- ap_run_insert_filter
- ap_run_handler
- ap_run_open_htaccess
- ap_run_rewrite_args



## MPM

MPM, 即  Multi-Processing Module，负责实现多进程/多线程等多处理器模型，实现服务器的高并发。

mpm 有若干个实现，不同操作系统支持不同的并发模型，适用不同的mpm

- prefork
- event
- worker
- ...

每个 mpm 都实现对以下hook的触发

- ap_run_child_status
- ap_run_child_init
- ap_run_drop_privileges
- ap_run_pre_mpm
- connection
  - ap_run_suspend_connection
  - ap_run_resume_connection
  - ap_run_create_connection
  - ap_run_pre_connection
  - ap_run_process_connection
- pending
  - ap_run_output_pending
  - ap_run_input_pending

其中， ap_run_child_init 用于通知其他模块，当前处于 子进程/子线程 初始化阶段。



## request

- aaa
  - ap_run_access_checker
  - ap_run_access_checker_ex
  - ap_run_force_authn
  - ap_run_check_user_id
  - ap_run_auth_checker
- translate
  - ap_run_pre_translate_name
  - ap_run_translate_name
- ap_run_post_perdir_config
- ap_run_map_to_storage
- ap_run_header_parser
- ap_run_type_checker
- ap_run_fixups
- ap_run_dirwalk_stat
- ap_run_create_request



## http_request

- ap_run_quick_handler
- ap_run_input_pending
- ap_run_create_request
- ap_run_post_read_request

## mod_proxy

### proxt_util.c

- ap_run_create_connection
- ap_run_pre_connection

### mod_proxy_connect

- ap_run_create_connection
- ap_run_pre_connection

### mod_proxy_ftp

- ap_run_create_connection
- ap_run_pre_connection

### mod_proxy_scgi

- ap_run_sub_req