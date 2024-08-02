// order_processor.h
/*************************************************************************
 * @file    order_processor.h
 * @brief   OrderProcessor class declaration for handling futures orders
 * @author  stanjiang
 * @date    2024-07-25
 * @copyright
***/

#ifndef _ORDER_SERVER_ORDER_PROCESSOR_H_
#define _ORDER_SERVER_ORDER_PROCESSOR_H_

#include <queue>
#include <mutex>
#include <unordered_map>
#include "futures_order.pb.h"
#include "role.pb.h"
#include "kafka_manager.h"

class OrderProcessor {
public:
    OrderProcessor();
    ~OrderProcessor();

    // Initialize the OrderProcessor
    int init();

    // Allocate user object
    void allocate_user_object(uint32_t account);

    // Process pending orders
    void process_orders();

    // Process a new incoming order and return the response
    cs_proto::OrderResponse process_new_order(const cs_proto::FuturesOrder& order);

    // Validate login request and return response
    cspkg::AccountLoginRes validate_login(const cspkg::AccountLoginReq& login_req);

private:
    // Send order to matching engine
    void send_order_to_matching(const cs_proto::FuturesOrder& order);

    // Match buy and sell orders
    void match_orders();

    std::queue<cs_proto::FuturesOrder> buy_orders_;
    std::queue<cs_proto::FuturesOrder> sell_orders_;
    std::mutex order_mutex_;
    KafkaManager& kafka_manager_;

    // Map to store user sessions
    std::unordered_map<uint32_t, std::string> user_sessions_;
    std::mutex session_mutex_;
};

#endif // _ORDER_SERVER_ORDER_PROCESSOR_H_