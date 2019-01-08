#include "rttp_async_ping_server.h"

#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DATA_SIZE 2000


struct uv_server_context : public rtsocket_server_context
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
	uv_server_context *ctx_ptr = (uv_server_context *)handle->data;

	if (nread <= 0)
		return;
    
	RTSOCKET s = rt_incoming_packet(buf->base, nread, (const char*)addr, sizeof(struct sockaddr), ctx_ptr);
	if (s != NULL && ctx_ptr->connected_socket_map.find(s) == ctx_ptr->connected_socket_map.end()) {
		cout << "incoming socket " << s << endl;
		std::shared_ptr<socket_io_info> ptr(new socket_io_info());
		ctx_ptr->connected_socket_map[s] = ptr;

		int mode = RTSM_LOW_LATENCY;
		rt_setsockopt(s, RTSO_MODE, (char*)&mode, sizeof(mode));
	}
}

void udp_send_callback(uv_udp_send_t* req, int status)
{
	uv_server_context *ctx_ptr = (uv_server_context *)req->data;
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
    uv_server_context *ctx_ptr = (uv_server_context *)handle->data;
    
    static int64_t last_statistic_time = 0;
    
	rt_tick();
    
    int64_t cur_time = get_micro_second();
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
        else {
            ctx_ptr->state[0] = 0;
        }
    }
}


void my_packet_send_imp(RTSOCKET socket, const char * data, int len, const char * sa, int sock_len)
{
	uv_server_context *ctx_ptr = (uv_server_context *)rt_get_userdata(socket);

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

int server_main_func(uv_server_context* ctx_ptr, int port)
{
	init_socket();
	
    ctx_ptr->socket = create_udp_socket(4*1024*1024);
    
	//uv_signal_t sigint;   //signal handle type
	rt_init(NULL, 0);
	rt_set_callback(on_rtsocket_event, my_packet_send_imp);

	struct sockaddr_in server_addr;

	uv_loop_t *loop;    //loop data type
	uv_timer_t timer;

	loop = uv_default_loop();     //Returns the initialized default loop
	//uv_signal_init(loop, &sigint);//Initialize the handle

	//Start the handle with the given callback, watching for the given signal.
	//uv_signal_start(&sigint, sigint_handler, SIGINT);

	uv_ip4_addr("0.0.0.0", port, &server_addr);

    int ret = uv_udp_init(loop, &ctx_ptr->udp_handle);   //Initialize a new UDP handle
    ret = uv_udp_open(&ctx_ptr->udp_handle, ctx_ptr->socket);
    
	ctx_ptr->udp_handle.data = ctx_ptr;
									  //Bind the UDP handle to an IP address and port.
	ret = uv_udp_bind(&ctx_ptr->udp_handle, (struct sockaddr *)&server_addr, 0);
    if (ret != 0) {
        cout<<"bind port failed"<<endl;
        return 0;
    }
    
    uv_timer_init(loop, &timer);
    timer.data = ctx_ptr;
    
	uv_timer_start(&timer, timer_callback, 5, 5);

	//Prepare for receiving data. 
	uv_udp_recv_start(&ctx_ptr->udp_handle, alloc_recv_buf, udp_receive_callback);
	//runs the event loop.
	uv_run(loop, UV_RUN_DEFAULT);

    close_socket(ctx_ptr->socket);
    
	return 0;

}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		cout << "usage: program <listen port>";
		return 0;
	}

	uv_server_context ctx;

	u_short port = atoi(argv[1]);

	std::thread t1(server_main_func, &ctx, port);

	while (ctx.run) {
		this_thread::sleep_for(std::chrono::milliseconds(1000));
		cout << "conn num: " << ctx.connection_num
			<< " waiting send num: " << ctx.total_waiting_send_num
			<< " state: " << ctx.state << endl;
	}


	t1.join();

	return 0;
}
