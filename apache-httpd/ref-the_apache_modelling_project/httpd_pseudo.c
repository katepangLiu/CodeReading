int flag = 1;
int mutex_listener;
int socket_listner;
int keep_alive;

int main() {
	init_first_start();
	
	while( flag ) {
		init_restart();

		flag = run_mpm();
	}
}

//multitask-process-module
int run_mpm() {
	int signal;
	while( signal ) {
		signal = watch_signal();
		keep_idle_child_nums();
	}

	return flag; //from signal;
}

void keep_idle_child_nums() {
	if( need_create() ) {
		create_child();
	}
	if( need_kill() ) {
		kill_child();
	}
}

void create_child() {

	init();
	request_response_loop();
}

void request_response_loop() {
	int signal;
	while( signal ) {
		signal = watch_master_signal();

		lock( mutex_listener );

		//blocking, wait for connection
		int socket_comm = accept( socket_listner );
		keep_alive_loop();

		un_lock( mutex_listener );
	}
}

void keep_alive_loop(socket_comm) {
	init_connect(socket_comm);
	while( keep_alive ) {
		int request = read_request(socket_comm);

		// hook: 通知modules作处理前的准备, 确定 handler
		// 如果一个module 认为自己能处理，则把自己的handler填充到 request
		post_read_request(request); 

		handle_request(request);
	}
}

void handle_request(int request) {

	//预处理: 完成从 URL 到 文件的映射
	//modify Request-URI:
	ap_getparents();
	ap_unescape_url(); 
	ap_location_walk();
	translate_name(); //hook
	map_to_storage(); //hook

	//AAA 权限控制;

	//mime type ===> handler
	type_checker(request);

	int handler = get_handler(request);
	int reponse = run_handler(handler, request);

	reponse = outputfilter(reponse);
}

int proxy_handler(int request) {

	//apache 也是用 hook 来实现 proxy 分发;
	//所有的proxy 注册hook scheme_handler
	// proxy_run_scheme_handler 会依次调用，直到一个成功;
	//每个scheme_handler 在内部判断自己是否能处理;

	int scheme = get_scheme(request);
	switch( scheme ) {
		case 1:
			return http_handler();
			break;
		case 2:
			return fcgi_handler();
			break;
		//...
	}
}