//
//  PingClient.h
//  librttpdemo
//
//  Created by gushenghua on 2018/10/21.
//  Copyright © 2018年 rtttech. All rights reserved.
//

#ifndef PingClient_h
#define PingClient_h

#include <stdio.h>

#if defined(__cplusplus)
extern "C"
{
#endif

void startPing(const char* server, int rttp_port, int tcp_port);
int getRttpLatency(void);
int getTcpLatency(void);
void stopPing(void);

#if defined(__cplusplus)
}
#endif
    
#endif /* PingClient_h */
