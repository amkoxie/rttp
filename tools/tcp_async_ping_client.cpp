#include "tcp_async_ping_client.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[])
{
	if (argc != 3) {
		std::cout << "usage: program <remote ip> <remote port>" << std::endl;
		return 0;
	}

	tcp_client_context ctx;

	std::thread t(tcp_client_main_func, &ctx, argv[1], atoi(argv[2]));

	while (ctx.run) {
		std::cout << "\rping: " << ctx.avg_latency/1000<<" milli seconds";
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	t.join();

	return 0;
}
