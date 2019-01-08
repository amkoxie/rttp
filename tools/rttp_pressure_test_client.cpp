#include "../api/rtsocket.h"
#include "../example/rttp_async_ping_client.h"
#include "tcp_async_ping_client.h"
#include <uv.h>
#include <thread>
#include <iostream>
#include <chrono>
#include <fstream>

using namespace std;


#define MAX_DATA_SIZE 2000


struct uv_client_context : public rtsocket_client_context
{
    bool sending = false;
    uv_udp_t udp_handle;
    
    uv_udp_send_t send_req;
    uv_buf_t send_buf;
};

void alloc_recv_buf(uv_handle_t *receive_handle, size_t suggested_size, uv_buf_t *buf)
{
    static char recv_buff[MAX_DATA_SIZE];
    
    buf->len = MAX_DATA_SIZE;
    buf->base = recv_buff;
}

void udp_receive_callback(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    uv_client_context *ctx_ptr = (uv_client_context *)handle->data;
    
    if (nread <= 0)
        return;
    
    RTSOCKET s = rt_incoming_packet(buf->base, nread, (const char*)addr, sizeof(struct sockaddr), ctx_ptr);
    
}

void udp_send_callback(uv_udp_send_t* req, int status)
{
    uv_client_context *ctx_ptr = (uv_client_context *)req->data;
    ctx_ptr->sending = false;
    
    ctx_ptr->udp_send_deq.pop_front();
    
    if (ctx_ptr->udp_send_deq.size() > 0) {
        ctx_ptr->sending = true;
        ctx_ptr->send_req.data = ctx_ptr;
        ctx_ptr->send_buf = uv_buf_init(ctx_ptr->udp_send_deq[0]->buffer, ctx_ptr->udp_send_deq[0]->len);
        uv_udp_send(&ctx_ptr->send_req, &ctx_ptr->udp_handle, &ctx_ptr->send_buf, 1, (const struct sockaddr*)&ctx_ptr->udp_send_deq[0]->addr, udp_send_callback);
    }
}

void timer_callback(uv_timer_t *handle)
{
    uv_client_context *ctx_ptr = (uv_client_context *)handle->data;
    
    static int64_t last_statistic_time = 0;
    
    rt_tick();
    
    std::vector<RTSOCKET> sockets;
    sockets.reserve(ctx_ptr->socket_map.size());
    
    for(auto kv : ctx_ptr->socket_map) {
        sockets.push_back(kv.first);
    }
    
    static int count_left_per_second = 0;
    static int time_left = 0;//miliseconds
    
    if (sockets.size() > 0) {
        if (count_left_per_second == 0) {
            count_left_per_second = sockets.size()*2;//each connection 2 packet per seconds
            time_left = 1000;
        }
    
        int cur_count = 0;
        if (time_left > 0) {
            cur_count = 5*count_left_per_second/time_left;
        }
        else {
            cur_count = count_left_per_second;
        }
    
        if (cur_count > 0) {
            for (int i = 0; i < cur_count; ++i) {
                int rand_index = rand()%sockets.size();
                send_ping_packet(sockets[rand_index], 1);
            }
            count_left_per_second -= cur_count;
        }
        time_left -= 5;
    }
    
    int64_t cur_time = get_micro_second();
    if (cur_time - last_statistic_time >= 1000 * 1000) {
        last_statistic_time = cur_time;
        ctx_ptr->connected_socket_num = ctx_ptr->socket_map.size();
    }
}


void my_packet_send_imp(RTSOCKET socket, const char * data, int len, const char * sa, int sock_len)
{
    uv_client_context *ctx_ptr = (uv_client_context *)rt_get_userdata(socket);
    
    std::shared_ptr<udp_pkt_send_item> ptr(new udp_pkt_send_item(data, len, (struct sockaddr *)sa, sock_len));
    ctx_ptr->udp_send_deq.push_back(ptr);
    
    if (!ctx_ptr->sending) {
        ctx_ptr->sending = true;
        ctx_ptr->send_req.data = ctx_ptr;
        ctx_ptr->send_buf = uv_buf_init(ctx_ptr->udp_send_deq[0]->buffer, ctx_ptr->udp_send_deq[0]->len);
        int ret = uv_udp_send(&ctx_ptr->send_req, &ctx_ptr->udp_handle, &ctx_ptr->send_buf, 1, (const struct sockaddr*)&ctx_ptr->udp_send_deq[0]->addr, udp_send_callback);
        if (ret != 0) {
            cout<<"uv_udp_send return "<<ret<<endl;
            udp_send_callback(&ctx_ptr->send_req, 0);
        }
    }
}

int rttp_pressure_test_func(uv_client_context* ctx_ptr, const char* remote_addr, int port, int conn_num)
{
    init_socket();
    
    ctx_ptr->socket = create_udp_socket(4*1024*1024);
    
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    
#if defined(SOCKADDR_WITH_LEN)
    sa.sin_len = sizeof(sa);
#endif
    
    sa.sin_family = AF_INET;
    std::string remote_ip = get_host_ip_address(remote_addr);
    if (remote_ip.size() == 0) {
        ctx_ptr->run = false;
        return 0;
    }
    else {
        //std::cout << "ip: " << remote_ip << std::endl;
    }
    
    sa.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    sa.sin_port = htons(port);
    
    if (sa.sin_addr.s_addr == -1) {
        ctx_ptr->run = false;
        return 0;
    }
    
    //uv_signal_t sigint;   //signal handle type
    rt_init(NULL, 0);
    rt_set_callback(on_rtsocket_event, my_packet_send_imp);
    
    uv_loop_t *loop;    //loop data type
    uv_timer_t timer;
    
    loop = uv_default_loop();     //Returns the initialized default loop

    int ret = uv_udp_init(loop, &ctx_ptr->udp_handle);   //Initialize a new UDP handle
    ret = uv_udp_open(&ctx_ptr->udp_handle, ctx_ptr->socket);
    
    ctx_ptr->udp_handle.data = ctx_ptr;
    
    uv_timer_init(loop, &timer);
    timer.data = ctx_ptr;
    
    uv_timer_start(&timer, timer_callback, 5, 5);
    
    for (int i = 0; i < conn_num; ++i) {
        RTSOCKET client_rtsocket = rt_socket(0);
        rt_set_userdata(client_rtsocket, ctx_ptr);
        rt_connect(client_rtsocket, (const char*)&sa, sizeof(sa));
    }
    
    //Prepare for receiving data.
    uv_udp_recv_start(&ctx_ptr->udp_handle, alloc_recv_buf, udp_receive_callback);
    //runs the event loop.
    uv_run(loop, UV_RUN_DEFAULT);
    
    std::unordered_map<RTSOCKET, std::shared_ptr<socket_io_info>>::iterator iter;
    for (iter = ctx_ptr->socket_map.begin(); iter != ctx_ptr->socket_map.end(); ++iter) {
        rt_close(iter->first);
    }
    ctx_ptr->socket_map.clear();
    ctx_ptr->latency_deq.clear();
    ctx_ptr->sent_packet_info_map.clear();
    
    close_socket(ctx_ptr->socket);
    ctx_ptr->socket = -1;
    
    return 0;
    
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
		cout << "usage: program <remote address> <rttp port> <connection num>" << endl;
		return 0;
	}

	uv_client_context rttp_ctx;
	
	std::thread t1(rttp_pressure_test_func, &rttp_ctx, argv[1], atoi(argv[2]), atoi(argv[3]));
	
	while (rttp_ctx.run) {
		uint64_t rttp_latency = rttp_ctx.avg_latency / 1000;

		if (rttp_ctx.avg_latency > 0) {
			cout << "\rrttp: " << rttp_latency << " ms, connection num: "<<rttp_ctx.connected_socket_num<<"      "<<flush;
			
		}
		this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	rttp_ctx.run = false;
	
	t1.join();

	cout << "process exit " << endl;

	return 0;
}
