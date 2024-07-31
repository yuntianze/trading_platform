/*************************************************************************
 * @file    order_server.h
 * @brief   OrderServer class declaration
 * @author  stanjiang
 * @date    2024-07-25
 * @copyright
***/

#ifndef _ORDER_SERVER_ORDER_SERVER_H_
#define _ORDER_SERVER_ORDER_SERVER_H_

#include <uv.h>
#include "kafka_manager.h"
#include "order_processor.h"

enum ServerStartModel {
    SERVER_START_NODAEMON = 0,
    SERVER_START_DAEMON = 1,
    SERVER_START_INVALID
};

enum SvrRunFlag {
    RUN_INIT = 0,
    RELOAD_CFG = 1,
    SERVER_EXIT = 2,
};

class OrderServer {
public:
    ~OrderServer();

    static OrderServer& instance();

    int init(ServerStartModel model);
    void run();
    void reload_config();
    void stop();

private:
    OrderServer();
    OrderServer(const OrderServer&) = delete;
    OrderServer& operator=(const OrderServer&) = delete;

    int init_daemon(ServerStartModel model);
    void process_run_flag();
    void perform_periodic_checks();

    static void on_async(uv_async_t* handle);
    static void on_timer(uv_timer_t* handle);

    static void sigusr1_handle(int sigval);
    static void sigusr2_handle(int sigval);

    uv_loop_t* loop_;
    uv_async_t async_handle_;
    uv_timer_t check_timer_;

    SvrRunFlag run_flag_;  
    KafkaManager& kafka_manager_;
    OrderProcessor order_processor_;
};

#endif // _ORDER_SERVER_ORDER_SERVER_H_