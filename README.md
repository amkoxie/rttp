RTTP - A Reliable Realtime Transport Protocol
==================================================

RTTP is a reliable UDP based application level data transport protocol designed for time critical applications which require timely delivery of information, such as  online game, live streaming, financial order routing, long distance data synchronization, etc.   It’s purpose is to provide a simple and robust network communication layer on top of  UDP (not limited to UDP). The functions it provides is same with TCP but with better performance on latency and speed.

# Features:

Low latency. TCP’s latency is serval times of RTTP’s at packet loss environments.

High throughput. Serval times faster than TCP.

Reliable. RTTP provides a reliable stream based delivery service, unlike TCP, IP/port change would not corrupt the connection.

High performance. Several thousands of long connections per CPU core with about 3K-BYTES/Second data transfer on each connection. 

Easy to integrate. Independent of socket IO framework/mode.

Cross-Platform. Windows/Linux/Mac/IOS/Android.

# Benchmark:
![TCP VS RTTP - 10% packet loss](http://www.rtttech.com/img/10lost.png)
![TCP VS RTTP - 20% packet loss](http://www.rtttech.com/img/20lost.png)

# Document:

http://www.rtttech.com/rttp/doc/index.html

# Getting Started

## Folder structure:
api: RTTP static/dynamic lib and C header file

example: client and server demo

tools: tools used to test RTTP

android_demo: RTTP android demo 

ios_demo: RTTP ios demo

unity3d_demo: Unity3D demo

## Build examples and tools:
### Requirements: 
cmake(https://cmake.org/): required for all platforms

gperftools(https://github.com/gperftools/gperftools): required for Linux/Mac

boost: https://www.boost.org/

libuv: http://libuv.org/

### Windows:
run build_win.bat

### Linux/Mac:
run build_posix.sh

# License:
Non-commercial use is free. 

Commercial use please contact us. 

# Contact:
Mail: support@rtttech.com
QQ Group: 645284582
