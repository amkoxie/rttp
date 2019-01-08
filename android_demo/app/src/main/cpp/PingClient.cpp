//
//  PingClient.c
//  librttpdemo
//
//  Created by gushenghua on 2018/10/21.
//  Copyright © 2018年 rtttech. All rights reserved.
//

#include "PingClient.h"
#include "../../../../../api/rtsocket.h"
#include "../../../../../example/rttp_async_ping_client.h"
#include "../../../../../tools/tcp_async_ping_client.h"
#include <thread>

static rtsocket_client_context gRttpClientCtx;
static tcp_client_context gTcpClientCtx;
static bool gStarted;

void startPing(const char* server, int rttp_port, int tcp_port)
{
    if (!gStarted) {
        gStarted = true;
        gRttpClientCtx.run = true;
        gTcpClientCtx.run  = true;
        gRttpClientCtx.thread_ptr = new std::thread(rttp_client_main_func, &gRttpClientCtx, server, rttp_port);
        gTcpClientCtx.thread_ptr = new std::thread(tcp_client_main_func, &gTcpClientCtx, server, tcp_port);
    }
}

int getRttpLatency(void)
{
    return gRttpClientCtx.avg_latency;
}

int getTcpLatency(void)
{
    return gTcpClientCtx.avg_latency;
}

void stopPing(void)
{
    if (gStarted) {

        gRttpClientCtx.run = false;
        gTcpClientCtx.run  = false;

        gRttpClientCtx.thread_ptr->join();
        gTcpClientCtx.thread_ptr->join();

        delete gRttpClientCtx.thread_ptr;
        gRttpClientCtx.thread_ptr = NULL;
        delete gTcpClientCtx.thread_ptr;
        gTcpClientCtx.thread_ptr = NULL;

        gStarted = false;
    }
    
}
