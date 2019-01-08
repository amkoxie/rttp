#include "rttp_async_ping_client.h"
#include <string>
#include <thread>
#include <chrono>

using namespace std;

int main(int argc, char* argv[])
{
	
	if (argc != 3) {
		cout << "usage: program <remote ip> <remote port>" << endl;
		return 0;
	}

	rtsocket_client_context ctx;

	std::thread t(rttp_client_main_func, &ctx, argv[1], atoi(argv[2]));

	while (ctx.run) {
		cout << "rttp: " << ctx.avg_latency / 1000 << " milli seconds " << ctx.state << endl;

		this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	t.join();

	return 0;
}
