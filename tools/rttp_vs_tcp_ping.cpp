#include "../api/rtsocket.h"
#include "../example/rttp_async_ping_client.h"
#include "tcp_async_ping_client.h"
#include <thread>
#include <iostream>
#include <chrono>
#include <fstream>

using namespace std;

int main(int argc, char* argv[])
{
	if (argc != 4) {
		cout << "usage: program <remote address> <rttp port> <tcp port>" << endl;
		return 0;
	}

	rtsocket_client_context rttp_ctx;
	tcp_client_context tcp_ctx;

	std::thread t1(rttp_client_main_func, &rttp_ctx, argv[1], atoi(argv[2]));
	std::thread t2(tcp_client_main_func, &tcp_ctx, argv[1], atoi(argv[3]));

	std::ofstream ofs("tcp_vs_rttp_result.csv");
	ofs << "TCP," << "RTTP" << std::endl;

	while (rttp_ctx.run && tcp_ctx.run) {
		uint64_t tcp_latency = tcp_ctx.avg_latency / 1000;
		uint64_t rttp_latency = rttp_ctx.avg_latency / 1000;

		if (tcp_ctx.avg_latency > 0 && rttp_ctx.avg_latency > 0) {
			cout << "tcp: " << tcp_latency << " ms, rttp: " << rttp_latency << " ms"<<endl<<flush;
			ofs << tcp_latency << "," << rttp_latency << std::endl;
		}
		this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	ofs.close();
	
	rttp_ctx.run = false;
	tcp_ctx.run = false;
	
	t1.join();
	t2.join();

	return 0;
}
