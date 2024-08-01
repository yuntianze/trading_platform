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
#include "futures_order.pb.h"
#include "kafka_manager.h"

class OrderProcessor {
public:
    OrderProcessor();
    ~OrderProcessor();

    // Initialize the OrderProcessor
    int init();

    // Process pending orders
    void process_orders();

    // Process a new incoming order and return the response
    cs_proto::OrderResponse process_new_order(const cs_proto::FuturesOrder& order);

private:
    // Send order to matching engine
    void send_order_to_matching(const cs_proto::FuturesOrder& order);

    // Match buy and sell orders
    void match_orders();

    std::queue<cs_proto::FuturesOrder> buy_orders_;
    std::queue<cs_proto::FuturesOrder> sell_orders_;
    std::mutex order_mutex_;
    KafkaManager& kafka_manager_;
};

#endif // _ORDER_SERVER_ORDER_PROCESSOR_H_