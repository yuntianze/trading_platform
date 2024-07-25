/*************************************************************************
 * @file    tcp_server.h
 * @brief   TcpServer class declaration
 * @author  stanjiang
 * @date    2024-07-20
 * @copyright
***/

#ifndef _GATEWAY_SERVER_TCP_SERVER_H_
#define _GATEWAY_SERVER_TCP_SERVER_H_

#include <uv.h>
#include "tcp_connect_mgr.h"

// Server start modes
enum ServerStartModel {
    SERVER_START_NODAEMON = 0,
    SERVER_START_DAEMON = 1,
    SERVER_START_INVALID
};

class TcpServer {
public:
    ~TcpServer();

    // Get the singleton instance of TcpServer
    static TcpServer& instance();

    // Initialize the server
    int init(ServerStartModel model);

    // Run the server main loop
    void run();

    // Get the uv loop
    uv_loop_t* get_loop() { return loop_; }

private:
    TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // Initialize the server as a daemon
    int init_daemon(ServerStartModel model);

    // Callback for new connections
    static void on_new_connection(uv_stream_t* server, int status);

    // Signal handlers
    static void sigusr1_handle(int sigval);
    static void sigusr2_handle(int sigval);

    uv_loop_t* loop_;        // Main event loop
    uv_tcp_t server_;        // TCP server handle
    TcpConnectMgr* conn_mgr_; // Connection manager
};

#endif  // _GATEWAY_SERVER_TCP_SERVER_H_

