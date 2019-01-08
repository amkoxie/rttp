cmake_minimum_required(VERSION 2.8)

#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_BINARY_DIR "../../bin")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS})

include_directories(../api)

add_executable(RttpSelectPingServer rttp_async_ping_server.cpp)
add_executable(RttpPingClient rttp_async_ping_client.cpp)
add_executable(RttpFileServer rttp_asio_file_server.cpp)
add_executable(RttpGetFile rttp_async_file_client.cpp)
add_executable(RttpUVPingServer rttp_libuv_ping_server.cpp)


IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    target_link_libraries(RttpPingClient ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib)
    target_link_libraries(RttpSelectPingServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib)
    target_link_libraries(RttpUVPingServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib libuv.lib)
    target_link_libraries(RttpFileServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib)
    target_link_libraries(RttpGetFile ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/RttpLibMD.lib ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libtcmalloc_minimalMD.lib ws2_32.lib)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        include_directories(/usr/local/include)
        target_link_libraries(RttpPingClient ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a)
        target_link_libraries(RttpSelectPingServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a)
        target_link_libraries(RttpUVPingServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a /usr/local/lib/libuv.a)
        target_link_libraries(RttpFileServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a /usr/local/lib/libboost_system.a /usr/local/lib/libboost_filesystem.a)
        target_link_libraries(RttpGetFile ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a /usr/local/lib/libtcmalloc.a)
    ELSE(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(RttpPingClient ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a)
        target_link_libraries(RttpSelectPingServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a)
        target_link_libraries(RttpUVPingServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a libuv.a)
        target_link_libraries(RttpFileServer ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a boost_system boost_filesystem)
        target_link_libraries(RttpGetFile ${CMAKE_CURRENT_SOURCE_DIR}/../api/${CMAKE_SYSTEM_NAME}${BITS}/libRttpLib.a libtcmalloc.a)
    ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

