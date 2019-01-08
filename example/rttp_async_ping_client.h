#pragma once

#include "../api/rtsocket.h"
#include "os_common.h"

#include <iostream>
#include <deque>
#include <set>
#include <map>
#include <unordered_map>
#include <assert.h>
#include <string.h>
#include <random>
#include <time.h>
#include <memory>
#include <numeric>
#include <thread>
#include <condition_variable>

using namespace std;

struct rtsocket_client_context
{
	std::thread *thread_ptr = NULL;
	bool run = true;
	SOCKET socket = -1;
	uint64_t total_received_udp_pkt = 0;
	uint64_t connected_socket_num = 0;
	std::deque<uint32_t> latency_deq;
	uint64_t avg_latency = 0;

	std::deque<std::shared_ptr<udp_pkt_send_item>> udp_send_deq;
	std::deque<std::shared_ptr<udp_pkt_send_item>> udp_recv_deq;

	std::unordered_map<RTSOCKET, std::shared_ptr<socket_io_info>> socket_map;
	std::unordered_map<uint32_t, sent_packet_info> sent_packet_info_map;

	char state[4096] = {0};
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
			if (item.send_buff_pos == item.send_buff_len) {
				si.send_deq.pop_front();
			}
		}
	}
}

inline void on_rtsocket_connect(RTSOCKET socket)
{
	//cout << "\nsocket " << socket << " connected" << endl;

	rtsocket_client_context *ctx_ptr = (rtsocket_client_context*)rt_get_userdata(socket);

	std::shared_ptr<socket_io_info> ptr(new socket_io_info());
	ctx_ptr->socket_map[socket] = ptr;
}


inline void on_rtsocket_read(RTSOCKET socket)
{
	rtsocket_client_context *ctx_ptr = (rtsocket_client_context*)rt_get_userdata(socket);
	socket_io_info &si = *ctx_ptr->socket_map[socket];

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
				ctx_ptr->socket_map.erase(socket);
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
		if (packet_size < 4 || packet_size > 4096) {
			std::cout << "rttp invalid packet size: " << packet_size << std::endl;
			ctx_ptr->run = false;
			return;
		}

		while (si.recv_buff_pos < packet_size) {
			int recv_bytes = rt_recv(socket, si.recv_buffer + si.recv_buff_pos, packet_size - si.recv_buff_pos, 0);
			if (recv_bytes == 0) {
				rt_close(socket);
				cout << "socket " << socket << " closed" << endl;
				ctx_ptr->socket_map.erase(socket);
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
			//cout << "received " << packet_size << " bytes" << endl;
			uint32_t packet_seq = *((uint32_t*)(si.recv_buffer + 4));

			sent_packet_info &spi = ctx_ptr->sent_packet_info_map[packet_seq];
			spi.resp_time = get_micro_second();
			si.recv_buff_pos = 0;
			ctx_ptr->latency_deq.push_back(spi.resp_time - spi.send_time);
			ctx_ptr->sent_packet_info_map.erase(packet_seq);

			if (ctx_ptr->latency_deq.size() > 10) {
				ctx_ptr->latency_deq.pop_front();
			}

			ctx_ptr->avg_latency = std::accumulate(ctx_ptr->latency_deq.begin(), ctx_ptr->latency_deq.end(), 0) / ctx_ptr->latency_deq.size();
		}
	}
}


inline void on_rtsocket_write(RTSOCKET socket)
{
	rtsocket_client_context *ctx_ptr = (rtsocket_client_context*)rt_get_userdata(socket);
	socket_io_info &si = *ctx_ptr->socket_map[socket];
	rtsocket_send_data(socket, si);
}



inline void on_rtsocket_error(RTSOCKET socket)
{
	rtsocket_client_context *ctx_ptr = (rtsocket_client_context*)rt_get_userdata(socket);

	int errcode = rt_get_error(socket);

	cout << "\nrttp socket " << socket << " error: " << errcode << endl;
	rt_close(socket);
	ctx_ptr->socket_map.erase(socket);
}

inline void on_rtsocket_event(RTSOCKET socket, int event)
{
	switch (event){
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
	rtsocket_client_context *ctx_ptr = (rtsocket_client_context*)rt_get_userdata(socket);

	int64_t send_start = get_micro_second();
	int send_ret = ::sendto(ctx_ptr->socket, data, len, 0, (struct sockaddr *)sa, sock_len);
	int64_t send_end = get_micro_second();
	//cout << "udp send take " << send_end - send_start << endl;
	//cout << "send return " << send_ret;
	if (send_ret < 0) {
		std::shared_ptr<udp_pkt_send_item> ptr(new udp_pkt_send_item(data, len, (struct sockaddr *)sa, sock_len));
		ctx_ptr->udp_send_deq.push_back(ptr);
		//cout << "udp send quue size: " << ctx_ptr->udp_send_deq.size() << endl;
	}
}

inline void send_ping_packet(RTSOCKET socket, int packet_num )
{
	rtsocket_client_context *ctx_ptr = (rtsocket_client_context*)rt_get_userdata(socket);

	socket_io_info &si = *ctx_ptr->socket_map[socket];

	if (si.send_deq.size() > 0 ||!rt_connected(socket))
		return;

	for (int i = 0; i < packet_num; ++i) {
		int packet_size[] = { 100, 300, 500, 800, 1100 };
		int rand_bytes = packet_size[rand() % (sizeof(packet_size) / sizeof(int))];
		int int_num = rand_bytes / 4;
		uint32_t *buffer = new uint32_t[int_num];
		buffer[0] = rand_bytes;

		static uint32_t s_packet_seq = 0;

		buffer[1] = ++s_packet_seq;

        /*
		for (int i = 2; i < int_num; ++i) {
			buffer[i] = i;
		}
        */

		int send_ret = rt_send(socket, (const char*)buffer, rand_bytes, 0);

		//cout << "send " << rand_bytes << " return " << send_ret << endl;
		sent_packet_info spi;
		spi.pkt_size = rand_bytes;
		spi.send_time = get_micro_second();
		ctx_ptr->sent_packet_info_map[s_packet_seq] = spi;

		if (send_ret < rand_bytes) {
			std::shared_ptr<socket_send_item> ptr(new socket_send_item((const char*)buffer, rand_bytes));
			si.send_deq.push_back(ptr);
			if (send_ret > 0) {
				ptr->send_buff_pos = send_ret;
			}
			break;
		}
		else {
			delete[] buffer;
		}
	}
}

inline void send_ping(rtsocket_client_context* ctx_ptr, uint64_t interval = 500*1000, int packet_num = 3)
{
	static uint64_t last_send_ping = 0;

	if (get_micro_second() - last_send_ping > interval) {
		last_send_ping = get_micro_second();
		std::unordered_map<RTSOCKET, std::shared_ptr<socket_io_info>>::iterator iter;
		for (iter = ctx_ptr->socket_map.begin(); iter != ctx_ptr->socket_map.end(); ++iter) {
			RTSOCKET rts = iter->first;
			send_ping_packet(rts, packet_num);
		}
	}
}

inline void do_udp_send(rtsocket_client_context* ctx_ptr, SOCKET socket)
{
	while (ctx_ptr->udp_send_deq.size() > 0) {
		udp_pkt_send_item &si = *ctx_ptr->udp_send_deq[0];

		int  send_bytes = ::sendto(socket, si.buffer, si.len, 0, (struct sockaddr*)&si.addr, si.addr_len);
		if (send_bytes == si.len) {
			ctx_ptr->udp_send_deq.pop_front();
		}
		else {
			assert(send_bytes < 0);
			break;
		}
	}
}

inline void do_udp_recv(rtsocket_client_context* ctx_ptr, SOCKET socket)
{
	char buff[2000] = { 0 };

	int i = 0;
	while (i++ < 1000) {
		struct sockaddr_in sa;
		socklen_t from_len = sizeof(sa);
		memset(&sa, 0, from_len);

		//cout << "start recv: ";
		int64_t recv_start = get_micro_second();
		int bytes = ::recvfrom(socket, buff, sizeof(buff), 0, (struct sockaddr*)&sa, &from_len);
		int64_t recv_end = get_micro_second();

		//cout << " recv take " << recv_end - recv_start << endl;

		//cout << "udp recv return " << bytes << endl;
		if (bytes > 0) {
			++ctx_ptr->total_received_udp_pkt;
			//cout << "total received: " << total_received_udp_pkt << endl;
			int64_t handle_start = get_micro_second();
			//if (rand() % 3 != 0) //packet lost test
			{
				RTSOCKET s = rt_incoming_packet(buff, bytes, (const char*)&sa, from_len, ctx_ptr);
				int64_t handle_end = get_micro_second();
				//cout << bytes << " bytes "<<" handle packet take " << handle_end - handle_start << endl;
			}
		}
		else {
			return;
		}
	}
}


inline int rttp_client_main_func(rtsocket_client_context* ctx_ptr, const char* remote_addr, int port)
{
	init_socket();

	ctx_ptr->socket = create_udp_socket();

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


	uint64_t last_periodical_time = 0;
	uint64_t last_print_latency = 0;

	rt_set_callback(on_rtsocket_event, packet_send_imp);

	RTSOCKET client_rtsocket;
	
	client_rtsocket = rt_socket(RTSM_LOW_LATENCY);
	
	rt_set_userdata(client_rtsocket, ctx_ptr);
	rt_connect(client_rtsocket, (const char*)&sa, sizeof(sa));

	while (ctx_ptr->run) {

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

		int waitms = 5;
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
		if (cur_time - last_periodical_time >= 10000) {
			last_periodical_time = cur_time;
			rt_tick();
			send_ping(ctx_ptr, 500*1000, 3);
			int bytes = rt_state_desc(client_rtsocket, ctx_ptr->state, sizeof(ctx_ptr->state));
			if (bytes > 0 && bytes < sizeof(ctx_ptr->state)) {
				ctx_ptr->state[bytes] = 0;
			}
		}

	}

	std::unordered_map<RTSOCKET, std::shared_ptr<socket_io_info>>::iterator iter;
	for (iter = ctx_ptr->socket_map.begin(); iter != ctx_ptr->socket_map.end(); ++iter) {
		rt_close(iter->first);
	}
	ctx_ptr->socket_map.clear();
	ctx_ptr->latency_deq.clear();
	ctx_ptr->sent_packet_info_map.clear();
	

	close_socket(ctx_ptr->socket);
	ctx_ptr->socket = -1;

	cout << "rttp exit" << endl;

	return 0;
}
