#pragma once

#include "../api/rtsocket.h"
#include "../example/os_common.h"

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

struct tcp_client_context
{
    bool run = true;
    std::thread *thread_ptr = NULL;
    SOCKET socket = -1;

	std::shared_ptr<socket_io_info> socket_info_ptr;

    uint64_t total_received = 0;
    std::deque<uint32_t> latency_deq;
    uint64_t avg_latency = 0;
    std::map<uint32_t, sent_packet_info> sent_packet_info_map;
    
    std::mutex mtx;
};

inline void socket_send_data(tcp_client_context *ctx_ptr)
{
	socket_io_info& si = *ctx_ptr->socket_info_ptr;
	while (si.send_deq.size() > 0) {
		socket_send_item& item = *si.send_deq[0];

		int send_bytes = send(ctx_ptr->socket, item.send_buffer + item.send_buff_pos, item.send_buff_len - item.send_buff_pos, 0);
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

inline void socket_recv_data(tcp_client_context *ctx_ptr)
{
	socket_io_info &si = *ctx_ptr->socket_info_ptr;
	while (true) {
		if (si.recv_buffer == NULL) {
			si.recv_buff_len = 4096;
			si.recv_buff_pos = 0;
			si.recv_buffer = new char[si.recv_buff_len];
		}


		while (si.recv_buff_pos < 4) {
			int recv_bytes = recv(ctx_ptr->socket, si.recv_buffer + si.recv_buff_pos, 4 - si.recv_buff_pos, 0);
			if (recv_bytes == 0) {
				ctx_ptr->run = false;
				cout << "socket " << ctx_ptr->socket << " closed" << endl;
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
			std::cout << "tcp invalid packet size: " << packet_size << std::endl;
			ctx_ptr->run = false;
			return;
		}

		while (si.recv_buff_pos < packet_size) {
			int recv_bytes = recv(ctx_ptr->socket, si.recv_buffer + si.recv_buff_pos, packet_size - si.recv_buff_pos, 0);
			if (recv_bytes == 0) {
				ctx_ptr->run = false;
				cout << "socket " << ctx_ptr->socket << " closed" << endl;
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

inline void send_ping_packet(tcp_client_context *ctx_ptr, int packet_num )
{

	socket_io_info &si = *ctx_ptr->socket_info_ptr;

	if (si.send_deq.size() > 0)
		return;

	for (int i = 0; i < packet_num; ++i) {
		int packet_size[] = { 100, 300, 500, 800, 1100 };
		int rand_bytes = packet_size[rand() % (sizeof(packet_size) / sizeof(int))];
		int int_num = rand_bytes / 4;
		uint32_t *buffer = new uint32_t[int_num];
		buffer[0] = rand_bytes;

		static uint32_t s_packet_seq = 0;

		buffer[1] = ++s_packet_seq;

		for (int i = 2; i < int_num; ++i) {
			buffer[i] = i;
		}

		int send_ret = send(ctx_ptr->socket, (const char*)buffer, rand_bytes, 0);

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

inline void send_ping(tcp_client_context* ctx_ptr, uint64_t interval = 500*1000, int packet_num = 3)
{
	static uint64_t last_send_ping = 0;

	if (get_micro_second() - last_send_ping > interval) {
		last_send_ping = get_micro_second();
		
		send_ping_packet(ctx_ptr, packet_num);
	}
}


inline int tcp_client_main_func(tcp_client_context* ctx_ptr, const char* remote_addr, int port)
{
	init_socket();

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
	
    ctx_ptr->socket = create_tcp_socket();
	int flag = 1;
	int result = setsockopt(ctx_ptr->socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	ctx_ptr->socket_info_ptr.reset(new socket_io_info());

	result = connect(ctx_ptr->socket, (const sockaddr*)&sa, sizeof(sa));
    bool connected = false;
    
	while (ctx_ptr->run) {

		fd_set rfds, wfds, efds;
		FD_ZERO(&rfds);
        FD_ZERO(&efds);
        FD_ZERO(&wfds);
		FD_SET(ctx_ptr->socket, &rfds);
        FD_SET(ctx_ptr->socket, &efds);
        FD_SET(ctx_ptr->socket, &wfds);
        
		fd_set* read_fs_ptr = &rfds;
		fd_set* write_fd_ptr = NULL;
		if (ctx_ptr->socket_info_ptr->send_deq.size() > 0 || !connected)
			write_fd_ptr = &wfds;

        fd_set* except_fd_ptr = &efds;
        
		int waitms = 5;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = waitms * 1000;
		int num = ::select(ctx_ptr->socket + 1, read_fs_ptr, write_fd_ptr, except_fd_ptr, &tv);
		if (num > 0) {
			if (FD_ISSET(ctx_ptr->socket, read_fs_ptr)) {
				socket_recv_data(ctx_ptr);
			}
			if (write_fd_ptr != NULL && FD_ISSET(ctx_ptr->socket, write_fd_ptr)) {
                connected = true;
				socket_send_data(ctx_ptr);
			}

			if (FD_ISSET(ctx_ptr->socket, except_fd_ptr)) {
				cout << "socket exception on tcp socket" << endl;
				ctx_ptr->run = false;
				break;
			}
		}

		uint64_t cur_time = get_micro_second();
		if (cur_time - last_periodical_time >= 10000) {
			last_periodical_time = cur_time;
			send_ping(ctx_ptr, 500*1000, 3);
		}

	}

	close_socket(ctx_ptr->socket);
	ctx_ptr->socket = -1;

	return 0;
}
