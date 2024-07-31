// order_processor.cpp
#include "order_processor.h"
#include "logger.h"

OrderProcessor::OrderProcessor() : kafka_manager_(KafkaManager::instance()) {
}

OrderProcessor::~OrderProcessor() {
}

int OrderProcessor::init() {
    // Perform any necessary initialization
    Logger::log(INFO, "OrderProcessor initialized");
    return 0;
}

void OrderProcessor::process_orders() {
    std::lock_guard<std::mutex> lock(order_mutex_);

    // Process buy orders
    while (!buy_orders_.empty()) {
        process_single_order(buy_orders_.front());
        buy_orders_.pop();
    }

    // Process sell orders
    while (!sell_orders_.empty()) {
        process_single_order(sell_orders_.front());
        sell_orders_.pop();
    }

    // Match orders after processing
    match_orders();
}

void OrderProcessor::process_new_order(const cs_proto::FuturesOrder& order) {
    std::lock_guard<std::mutex> lock(order_mutex_);
    if (order.side() == cs_proto::OrderSide::BUY) {
        buy_orders_.push(order);
        Logger::log(INFO, "Buy order added: Order ID {}", order.order_id());
    } else {
        sell_orders_.push(order);
        Logger::log(INFO, "Sell order added: Order ID {}", order.order_id());
    }
}

void OrderProcessor::process_single_order(const cs_proto::FuturesOrder& order) {
    // Process the order (e.g., validate, apply business rules)
    Logger::log(INFO, "Processing order: ID {}, Type {}, Quantity {}, Price {}",
                order.order_id(), order.type(), order.quantity(), order.price());

    // TODO: Implement order processing logic
    // For example:
    // - Validate the order
    // - Apply any business rules
    // - Update order status

    // Send order to matching engine
    send_order_to_matching(order);

    // Send order status update
    cs_proto::OrderStatusUpdate status_update;
    status_update.set_order_id(order.order_id());
    status_update.set_new_status(cs_proto::OrderStatus::ACCEPTED);
    send_order_status_update(status_update);
}

void OrderProcessor::match_orders() {
    // TODO: Implement order matching logic
    // This could involve:
    // - Comparing buy and sell orders
    // - Executing trades when orders match
    // - Updating order statuses
    // - Sending trade confirmations

    Logger::log(INFO, "Order matching completed");
}

void OrderProcessor::send_order_to_matching(const cs_proto::FuturesOrder& order) {
    kafka_manager_.produce("matching_orders_topic", order);
    Logger::log(INFO, "Order sent to matching engine: Order ID {}", order.order_id());
}

void OrderProcessor::send_order_status_update(const cs_proto::OrderStatusUpdate& status_update) {
    kafka_manager_.produce("order_status_topic", status_update);
    Logger::log(INFO, "Order status update sent: Order ID {}, New Status {}", 
                status_update.order_id(), cs_proto::OrderStatus_Name(status_update.new_status()));
}