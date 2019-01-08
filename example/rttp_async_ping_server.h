#include "rtsocket.h"
#include "os_common.h"
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

struct rtsocket_server_context {
	bool run = true;
	SOCKET socket = -1;
	uint64_t total_send = 0;
	uint64_t connection_num = 0;
	uint64_t total_waiting_send_num = 0;
	std::deque<std::shared_ptr<udp_pkt_send_item>> udp_send_deq;
	std::deque<std::shared_ptr<udp_pkt_send_item>> udp_recv_deq;
	std::unordered_map<RTSOCKET, std::shared_ptr<socket_io_info>> connected_socket_map;

	char state[4096] = { 0 };
};

inline void rtsocket_send_data(RTSOCKET socket, socket_io_info& si)
{
	while (si.send_deq.size() > 0) {
		socket_send_item& item = *si.send_deq[0];

		int send_bytes = rt_send(socket, item.send_buffer + item.send_buff_pos, item.send_buff_len - item.send_buff_pos, 0);
		if (send_bytes <= 0) {
			return;
		}
		else {
			item.send_buff_pos += send_bytes;
			//cout << "send " << send_bytes << " bytes, total send "<<item.send_buff_pos << endl;
			if (item.send_buff_pos == item.send_buff_len) {
				si.send_deq.pop_front();
			}
		}
	}
}


inline void on_rtsocket_connect(RTSOCKET socket)
{
	
}

inline void on_rtsocket_read(RTSOCKET socket)
{
	rtsocket_server_context *ctx_ptr = (rtsocket_server_context *)rt_get_userdata(socket);

	socket_io_info &si = *ctx_ptr->connected_socket_map[socket];

	while (true) {
		if (si.recv_buffer == NULL) {
			si.recv_buff_len = 4096;
			si.recv_buff_pos = 0;
			si.recv_buffer = new char[si.recv_buff_len];
		}


		while (si.recv_buff_pos < 4) {
			int recv_bytes = rt_recv(socket, si.recv_buffer + si.recv_buff_pos, 4 - si.recv_buff_pos, 0);
			if (recv_bytes == 0) {
				rt_close(socket);
				ctx_ptr->connected_socket_map.erase(socket);
				return;
			}
			else if (recv_bytes > 0) {
				si.recv_buff_pos += recv_bytes;
			}
			else {
				return;
			}
		}

		int packet_size = *((uint32_t*)si.recv_buffer);

		if (packet_size < 4 || packet_size > 3000) {
			cout << "received invalid packet size from rttp socket " << socket << endl;
			rt_close(socket);
			ctx_ptr->connected_socket_map.erase(socket);
			return;
		}

		while (si.recv_buff_pos < packet_size) {
			int recv_bytes = rt_recv(socket, si.recv_buffer + si.recv_buff_pos, packet_size - si.recv_buff_pos, 0);
			if (recv_bytes == 0) {
				rt_close(socket);
				ctx_ptr->connected_socket_map.erase(socket);
				return;
			}
			else if (recv_bytes > 0) {
				si.recv_buff_pos += recv_bytes;
			}
			else {
				return;
			}
		}

		if (si.recv_buff_pos == packet_size) {
            //cout<<"received "<<packet_size<<" bytes"<<endl;
			si.send_deq.push_back(std::shared_ptr<socket_send_item>(new socket_send_item(si.recv_buffer, packet_size)));
			si.recv_buffer = NULL;
			si.recv_buff_len = 0;
			si.recv_buff_pos = 0;

			rtsocket_send_data(socket, si);
		}
	}
}



inline void on_rtsocket_write(RTSOCKET socket)
{
	rtsocket_server_context *ctx_ptr = (rtsocket_server_context *)rt_get_userdata(socket);

	socket_io_info &si = *ctx_ptr->connected_socket_map[socket];
	
	rtsocket_send_data(socket, si);
}


inline void on_rtsocket_error(RTSOCKET socket)
{
	rtsocket_server_context *ctx_ptr = (rtsocket_server_context *)rt_get_userdata(socket);

	int errcode = rt_get_error(socket);
    cout<<"socket "<<socket<< " error " << errcode <<endl;
	rt_close(socket);
	ctx_ptr->connected_socket_map.erase(socket);
}

inline void on_rtsocket_event(RTSOCKET socket, int event)
{
	switch (event) {
	case RTTP_EVENT_CONNECT:
		on_rtsocket_connect(socket);
		return;
	case RTTP_EVENT_READ:
		on_rtsocket_read(socket);
		return;
	case RTTP_EVENT_WRITE:
		on_rtsocket_write(socket);
		return;
	case RTTP_EVENT_ERROR:
		on_rtsocket_error(socket);
		return;
	}
}

inline void packet_send_imp(RTSOCKET socket, const char * data, int len, const char * sa, int sock_len)
{
	rtsocket_server_context *ctx_ptr = (rtsocket_server_context *)rt_get_userdata(socket);

	//int64_t send_start = get_micro_second();
	int send_ret = ::sendto(ctx_ptr->socket, data, len, 0, (struct sockaddr *)sa, sock_len);
	//int64_t send_end = get_micro_second();
	//cout << "udp send take " << send_end - send_start << endl;
	if (send_ret < 0) {
		//cout << "udp send return " << send_ret << " error: " << get_last_error() << endl;
		std::shared_ptr<udp_pkt_send_item> ptr(new udp_pkt_send_item(data, len, (struct sockaddr *)sa, sock_len));
		ctx_ptr->udp_send_deq.push_back(ptr);
	}
	else {
		++ctx_ptr->total_send;
		//cout << "total send " << total_send << endl;
	}
}


inline void do_udp_send(rtsocket_server_context *ctx_ptr, SOCKET socket)
{
	while (ctx_ptr->udp_send_deq.size() > 0) {
		udp_pkt_send_item &si = *ctx_ptr->udp_send_deq[0];

		int  send_bytes = ::sendto(socket, si.buffer, si.len, 0, (struct sockaddr*)&si.addr, si.addr_len);
		if (send_bytes == si.len) {
			++ctx_ptr->total_send;
			//cout << "total send " << total_send << endl;
			ctx_ptr->udp_send_deq.pop_front();
		}
		else {
			assert(send_bytes < 0);
			break;
		}
	}
}

inline void do_udp_recv(rtsocket_server_context *ctx_ptr, SOCKET socket)
{
	char buff[2000] = { 0 };

	int i = 0;
	while (i++ < 1000) {
		struct sockaddr_storage sa = { 0 };
		socklen_t from_len = sizeof(sa);
		int bytes = ::recvfrom(socket, buff, sizeof(buff), 0, (struct sockaddr*)&sa, &from_len);

		if (bytes > 0) {
			RTSOCKET s = rt_incoming_packet(buff, bytes, (const char*)&sa, from_len, ctx_ptr);
			if (s != NULL && ctx_ptr->connected_socket_map.find(s) == ctx_ptr->connected_socket_map.end()) {
				cout << "incoming socket " << s << endl;
				std::shared_ptr<socket_io_info> ptr(new socket_io_info());
				ctx_ptr->connected_socket_map[s] = ptr;

				int mode = RTSM_LOW_LATENCY;
				rt_setsockopt(s, RTSO_MODE, (char*)&mode, sizeof(mode));
				
			}
		}
		else {
			return;
		}
	}
}