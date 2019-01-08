#include "rtsocket.h"
#include "os_common.h"
#include "rttp_async_ping_server.h"

#include <iostream>
#include <deque>
#include <set>
#include <map>
#include <assert.h>
#include <string.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include <condition_variable>

using namespace std;

int server_main_func(rtsocket_server_context* ctx_ptr, int port)
{
	init_socket();
	ctx_ptr->socket = create_udp_socket(4*1024*1024);
	
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	
	sa.sin_port = htons(port);

	int ret = ::bind(ctx_ptr->socket, (struct sockaddr*)&sa, sizeof(sa));
	if (ret == 0) {
		cout << "listen on port " << port << endl;
	}
	else {
		cout << "bind port " << port << " failed" << endl;
		return 0;
	}

	
	uint64_t last_notify_tick_time = 0;
	uint64_t last_statistic_time = 0;

	rt_set_callback(on_rtsocket_event, packet_send_imp);

	while (true) {

		do_udp_send(ctx_ptr, ctx_ptr->socket);

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(ctx_ptr->socket, &rfds);
		fd_set* read_fs_ptr = &rfds;

		fd_set* write_fs_ptr = NULL;
		fd_set wfds;
		FD_ZERO(&wfds);
		FD_SET(ctx_ptr->socket, &wfds);
		if (ctx_ptr->udp_send_deq.size() > 0) {
			write_fs_ptr = &wfds;
		}

		int waitms = 1;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = waitms * 1000;
		int num = ::select(ctx_ptr->socket + 1, read_fs_ptr, write_fs_ptr, NULL, &tv);
		if (num > 0) {
			if (FD_ISSET(ctx_ptr->socket, read_fs_ptr)) {
				do_udp_recv(ctx_ptr, ctx_ptr->socket);
			}
		}
		
		uint64_t cur_time = get_micro_second();
		if (cur_time - last_notify_tick_time >= 2000) {
			last_notify_tick_time = cur_time;
			//cout << "rt_tick" << endl;
			rt_tick();	
			//cout << "rt_tick end" << endl;
		}

		if (cur_time - last_statistic_time >= 1000 * 1000) {
			last_statistic_time = cur_time;

			uint32_t total = 0;
			std::unordered_map<RTSOCKET, std::shared_ptr<socket_io_info>>::iterator iter;
			for (iter = ctx_ptr->connected_socket_map.begin(); iter != ctx_ptr->connected_socket_map.end(); ++iter) {
				total += iter->second->send_deq.size();
			}
			ctx_ptr->total_waiting_send_num = total;
			ctx_ptr->connection_num = ctx_ptr->connected_socket_map.size();
			if (ctx_ptr->connected_socket_map.size() >= 1) {
				int bytes = rt_state_desc(ctx_ptr->connected_socket_map.begin()->first, ctx_ptr->state, sizeof(ctx_ptr->state));
				if (bytes > 0 && bytes < sizeof(ctx_ptr->state)) {
					ctx_ptr->state[bytes] = 0;
				}
			}
		}
	}

	close_socket(ctx_ptr->socket);

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		cout << "usage: program <listen port>";
		return 0;
	}

	rtsocket_server_context ctx;
	
	u_short port = atoi(argv[1]);

	std::thread t1(server_main_func, &ctx, port);

	while (ctx.run) {
		this_thread::sleep_for(std::chrono::milliseconds(1000));
		cout << "conn num: " << ctx.connection_num
			<< " waiting send num: " << ctx.total_waiting_send_num
			<< " state: "<<ctx.state << endl;
	}


	t1.join();

	return 0;
}
