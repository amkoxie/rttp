cmake_minimum_required(VERSION 2.8)

#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_BINARY_DIR "../../bin")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

message(${CMAKE_CURRENT_SOURCE_DIR})

math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")

#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS})


add_executable(TcpPingClient tcp_async_ping_client.cpp)
add_executable(TcpPingServer boost_tcp_echo_server.cpp)
add_executable(RttpVsTcpPing rttp_vs_tcp_ping.cpp)
add_executable(RttpPressureTestClient rttp_pressure_test_client.cpp)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    target_link_libraries(TcpPingClient ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib)
    target_link_libraries(RttpVsTcpPing  ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib)
    target_link_libraries(RttpPressureTestClient  ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib libuv.lib)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        add_definitions(-DSOCKADDR_WITH_LEN)
        include_directories(/usr/local/include)
        target_link_libraries(TcpPingClient ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a)
        target_link_libraries(RttpVsTcpPing  ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a)
        target_link_libraries(RttpPressureTestClient  ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a /usr/local/lib/libuv.a)
        target_link_libraries(TcpPingServer /usr/local/lib/libboost_system.a)
    ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(TcpPingClient ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a)
        target_link_libraries(RttpVsTcpPing  ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a)
        target_link_libraries(RttpPressureTestClient  ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a libuv.a)
        target_link_libraries(TcpPingServer boost_system)
    ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")


