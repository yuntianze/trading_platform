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
#include <atomic>
#include <string>
#include "tcp_connect_mgr.h"
#include "kafka_manager.h"


// Server start modes
enum ServerStartModel {
    SERVER_START_NODAEMON = 0,
    SERVER_START_DAEMON = 1,
    SERVER_START_INVALID
};

// Communication server running type
enum SvrRunFlag {
    RUN_INIT = 0,
    RELOAD_CFG = 1,
    TCP_EXIT = 2,
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

    // Reload the server configuration
    void reload_config();

    // Stop the server
    void stop();

    // Get the uv loop
    uv_loop_t* get_loop() { return loop_; }

private:
    TcpServer();
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // Initialize the server as a daemon
    int init_daemon(ServerStartModel model);

    // Process the server running flag
    void process_run_flag();

    // Perform periodic checks
    void perform_periodic_checks();

    // Callback for new connections
    static void on_new_connection(uv_stream_t* server, int status);

    // Callback for connection closure
    static void on_close(uv_handle_t* handle);

    // Signal handlers
    static void signal_handler(int signum);
    static void sigusr1_handle(int sigval);
    static void sigusr2_handle(int sigval);

    // Async and timer handlers
    static void on_async(uv_async_t* handle);
    static void on_timer(uv_timer_t* handle);

    // Kafka message handling
    void handle_kafka_message(const google::protobuf::Message& message);

    // Handle login response
    void handle_login_response(const cspkg::AccountLoginRes& login_res);

    // Handle order response
    void handle_order_response(const cs_proto::OrderResponse& order_res);

    uv_async_t async_handle_;  // Async handle for signal handling
    uv_timer_t check_timer_;   // Timer for checking connections

    uv_loop_t* loop_;   // Main event loop
    uv_tcp_t server_;   // TCP server handle
    TcpConnectMgr* conn_mgr_; // Connection manager
    std::atomic<SvrRunFlag> run_flag_;  // Server running flag
    KafkaManager& kafka_manager_;  // Kafka message manager
};

#endif  // _GATEWAY_SERVER_TCP_SERVER_H_