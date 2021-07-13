# 模块间的协作

## Overview

apache 提供了2个维度的 hook,  实现 事件通知，回调相应的handler

- **ap_run_xxx  将触发 所有  注册了 ap_hook_xxx 的 handler**
- **proxy_run_xxx 将触发 所有 注册了 proxy_hook_xxx 的 handler**

proxy_run_xxx 用于 mod_proxy 及其子模块之间的事件通知，
其他的场景下均使用 ap_run_xxx 来实现。



## ap_run_xxx

示例： 各个模块如何知道当前正在进行子进程的模块初始化

- main.c 通过调用 ap_run_mpm，触发  mpm 模块 prefork的 prefork_run，
  - 注：这里所有注册了 ap_hook_mpm 的模块 都会收到 通知；
  - 但是 仅允许加载一个 mpm
- prefork 中调用 child_main 
- child_main 中调用 ap_run_child_init, 触发  mod_core的 core_child_init 
  - 注：这里所有注册了 ap_hook_child_main 的模块都会收到通知。

通过一级一级的事件通知，通知各模块现在处于什么阶段，应该完成该阶段相应的工作。

*main.c*

```c
int main(int argc, const char *const *argv)
{
    //...
    do {
        //...
        ap_main_state = AP_SQ_MS_RUN_MPM;
        rc = ap_run_mpm(pconf, plog, ap_server_conf);

        apr_pool_lock(pconf, 0);

    } while (rc == OK);

    if (rc == DONE) {
        rc = OK;
    }
    else if (rc != OK) {
        ap_log_error(APLOG_MARK, APLOG_EMERG, 0, NULL, APLOGNO(02818)
                     "MPM run failed, exiting");
    }
    destroy_and_exit_process(process, rc);

    /* NOTREACHED */
    return !OK;
}
```

*prefork.c*

```c
static int prefork_run(apr_pool_t *_pconf, apr_pool_t *plog, server_rec *s)
{
    //...
    make_child(ap_server_conf, 0);
    //...
}

static int make_child(server_rec *s, int slot){
    child_main(slot, 0);
}

static void child_main(int child_num_arg, int child_bucket) {
	ap_run_child_init(pchild, ap_server_conf);   
}
```

*core.c*

```c
static void core_child_init(apr_pool_t *pchild, server_rec *s)
{
    apr_proc_t proc;

    /* The MPMs use plain fork() and not apr_proc_fork(), so we have to call
     * apr_random_after_fork() manually in the child
     */
    proc.pid = getpid();
    apr_random_after_fork(&proc);
}
```



## proxy_hook_xxx

示例：mod_proxy 分发给具体协议的proxy，连接后端服务。

*mod_proxy.c*

```c
static int proxy_handler(request_rec *r) {
    do {
        if( !dirct_connect ) {
            for (i = 0; i < proxies->nelts; i++) {
            	p2 = ap_strchr_c(ents[i].scheme, ':');  /* is it a partial URL? */
                if (strcmp(ents[i].scheme, "*") == 0 ||
                    (ents[i].use_regex &&
                     ap_regexec(ents[i].regexp, url, 0, NULL, 0) == 0) ||
                    (p2 == NULL && ap_cstr_casecmp(scheme, ents[i].scheme) == 0) ||
                    (p2 != NULL &&
                    ap_cstr_casecmpn(url, ents[i].scheme,
                                strlen(ents[i].scheme)) == 0)) {
                    
                	/* handle the scheme */
                    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, r, APLOGNO(01142)
                                  "Trying to run scheme_handler against proxy");

                    if (ents[i].creds) {
                        apr_table_set(r->notes, "proxy-basic-creds", ents[i].creds);
                        ap_log_rerror(APLOG_MARK, APLOG_TRACE1, 0, r,
                                      "Using proxy auth creds %s", ents[i].creds);
                    }

                    access_status = proxy_run_scheme_handler(r, worker,
                                                             conf, url,
                                                             ents[i].hostname,
                                                             ents[i].port);

                    if (ents[i].creds) apr_table_unset(r->notes, "proxy-basic-creds");
                }
            }
        }
	} while (!PROXY_WORKER_IS_USABLE(worker) &&
             max_attempts > attempts++);
}
```

*mod_proxy_http.c*

```c
static void ap_proxy_http_register_hook(apr_pool_t *p)
{
    ap_hook_post_config(proxy_http_post_config, NULL, NULL, APR_HOOK_MIDDLE);
    proxy_hook_scheme_handler(proxy_http_handler, NULL, NULL, APR_HOOK_FIRST);
    proxy_hook_canon_handler(proxy_http_canon, NULL, NULL, APR_HOOK_FIRST);
    warn_rx = ap_pregcomp(p, "[0-9]{3}[ \t]+[^ \t]+[ \t]+\"[^\"]*\"([ \t]+\"([^\"]+)\")?", 0);
}

/*
 * This handles http:// URLs, and other URLs using a remote proxy over http
 * If proxyhost is NULL, then contact the server directly, otherwise
 * go via the proxy.
 * Note that if a proxy is used, then URLs other than http: can be accessed,
 * also, if we have trouble which is clearly specific to the proxy, then
 * we return DECLINED so that we can try another proxy. (Or the direct
 * route.)
 */
static int proxy_http_handler(request_rec *r, proxy_worker *worker,
                              proxy_server_conf *conf,
                              char *url, const char *proxyname,
                              apr_port_t proxyport)
{
    //...

    /* Step One: Determine Who To Connect To */
    uri = apr_palloc(p, sizeof(*uri));
    if ((status = ap_proxy_determine_connection(p, r, conf, worker, backend,
                                            uri, &locurl, proxyname,
                                            proxyport, req->server_portstr,
                                            sizeof(req->server_portstr))))
        goto cleanup;

    /* The header is always (re-)built since it depends on worker settings,
     * but the body can be fetched only once (even partially), so it's saved
     * in between proxy_http_handler() calls should we come back here.
     */
    req->header_brigade = apr_brigade_create(p, req->bucket_alloc);
    if (input_brigade == NULL) {
        input_brigade = apr_brigade_create(p, req->bucket_alloc);
        apr_pool_userdata_setn(input_brigade, "proxy-req-input", NULL, p);
    }
    req->input_brigade = input_brigade;

    /* Prefetch (nonlocking) the request body so to increase the chance to get
     * the whole (or enough) body and determine Content-Length vs chunked or
     * spooled. By doing this before connecting or reusing the backend, we want
     * to minimize the delay between this connection is considered alive and
     * the first bytes sent (should the client's link be slow or some input
     * filter retain the data). This is a best effort to prevent the backend
     * from closing (from under us) what it thinks is an idle connection, hence
     * to reduce to the minimum the unavoidable local is_socket_connected() vs
     * remote keepalive race condition.
     */
    if ((status = ap_proxy_http_prefetch(req, uri, locurl)) != OK)
        goto cleanup;

    /* We need to reset backend->close now, since ap_proxy_http_prefetch() set
     * it to disable the reuse of the connection *after* this request (no keep-
     * alive), not to close any reusable connection before this request. However
     * assure what is expected later by using a local flag and do the right thing
     * when ap_proxy_connect_backend() below provides the connection to close.
     */
    toclose = backend->close;
    backend->close = 0;

    while (retry < 2) {
        if (retry) {
            char *newurl = url;

            /* Step One (again): (Re)Determine Who To Connect To */
            if ((status = ap_proxy_determine_connection(p, r, conf, worker,
                            backend, uri, &newurl, proxyname, proxyport,
                            req->server_portstr, sizeof(req->server_portstr))))
                break;

            /* The code assumes locurl is not changed during the loop, or
             * ap_proxy_http_prefetch() would have to be called every time,
             * and header_brigade be changed accordingly...
             */
            AP_DEBUG_ASSERT(strcmp(newurl, locurl) == 0);
        }

        /* Step Two: Make the Connection */
        if (ap_proxy_check_connection(scheme, backend, r->server, 1,
                                      PROXY_CHECK_CONN_EMPTY)
                && ap_proxy_connect_backend(scheme, backend, worker,
                                            r->server)) {
            ap_log_rerror(APLOG_MARK, APLOG_ERR, 0, r, APLOGNO(01114)
                          "HTTP: failed to make connection to backend: %s",
                          backend->hostname);
            status = HTTP_SERVICE_UNAVAILABLE;
            break;
        }

        /* Step Three: Create conn_rec */
        if ((status = ap_proxy_connection_create_ex(scheme, backend, r)) != OK)
            break;
        req->origin = backend->connection;

        /* Don't recycle the connection if prefetch (above) told not to do so */
        if (toclose) {
            backend->close = 1;
            req->origin->keepalive = AP_CONN_CLOSE;
        }

        /* Step Four: Send the Request
         * On the off-chance that we forced a 100-Continue as a
         * kinda HTTP ping test, allow for retries
         */
        status = ap_proxy_http_request(req);
        if (status != OK) {
            proxy_run_detach_backend(r, backend);
            if (req->do_100_continue && status == HTTP_SERVICE_UNAVAILABLE) {
                ap_log_rerror(APLOG_MARK, APLOG_INFO, status, r, APLOGNO(01115)
                              "HTTP: 100-Continue failed to %pI (%s)",
                              worker->cp->addr, worker->s->hostname_ex);
                backend->close = 1;
                retry++;
                continue;
            }
            break;
        }

        /* Step Five: Receive the Response... Fall thru to cleanup */
        status = ap_proxy_http_process_response(req);
        if (status == SUSPENDED) {
            return SUSPENDED;
        }
        if (req->backend) {
            proxy_run_detach_backend(r, req->backend);
        }

        break;
    }

    /* Step Six: Clean Up */
cleanup:
    if (req->backend) {
        if (status != OK)
            req->backend->close = 1;
        ap_proxy_release_connection(scheme, req->backend, r->server);
    }
    return status;
}
```

- Determine Who To Connect To
- Determine Who To Connect To
  - (Re)Determine Who To Connect To
- Create conn_rec
- Send the Request
- Receive the Response... Fall thru to cleanup
- Clean Up
