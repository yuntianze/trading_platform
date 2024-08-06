/*************************************************************************
 * @file    order_server.h
 * @brief   OrderServer class declaration
 * @author  stanjiang
 * @date    2024-07-25
 * @copyright
***/

#ifndef _ORDER_SERVER_ORDER_SERVER_H_
#define _ORDER_SERVER_ORDER_SERVER_H_

#include <atomic>
#include <string>
#include "kafka_manager.h"
#include "order_processor.h"


// Server start modes
enum ServerStartModel {
    SERVER_START_NODAEMON = 0,
    SERVER_START_DAEMON = 1,
    SERVER_START_INVALID
};

class OrderServer {
public:
    ~OrderServer();

    // Get the singleton instance of OrderServer
    static OrderServer& instance();

    // Initialize the server
    int init(ServerStartModel model);
    
    // Run the server
    void run();
    
    // Request configuration reload
    void reload_config();
    
    // Stop the server
    void stop();

private:
    OrderServer();
    OrderServer(const OrderServer&) = delete;
    OrderServer& operator=(const OrderServer&) = delete;

    // Initialize as daemon if required
    int init_daemon(ServerStartModel model);
    
    // Process server run flags
    void process_run_flag();
    
    // Handle incoming Kafka messages
    void handle_kafka_message(const google::protobuf::Message& message);

    // Handle login request
    void handle_login_request(const cspkg::AccountLoginReq& login_req);

    // Handle futures order
    void handle_futures_order(const cs_proto::FuturesOrder& order);

    // Signal handler
    static void signal_handler(int signum);

    std::atomic<bool> running_;        // Flag to control the main loop
    std::atomic<bool> reload_config_;  // Flag for configuration reload
    KafkaManager& kafka_manager_;      // Kafka manager instance
    OrderProcessor order_processor_;   // Order processor instance
    std::string order_to_gateway_topic_;  // Kafka topic for order messages
};

#endif // _ORDER_SERVER_ORDER_SERVER_H_