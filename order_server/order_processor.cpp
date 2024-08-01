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
        process_new_order(buy_orders_.front());
        buy_orders_.pop();
    }

    // Process sell orders
    while (!sell_orders_.empty()) {
        process_new_order(sell_orders_.front());
        sell_orders_.pop();
    }

    // Match orders after processing
    match_orders();
}

cs_proto::OrderResponse OrderProcessor::process_new_order(const cs_proto::FuturesOrder& order) {
    cs_proto::OrderResponse response;
    response.set_order_id(order.order_id());
    response.set_client_id(order.client_id());

    // Process the order (e.g., validate, apply business rules)
    Logger::log(INFO, "Processing order: ID {}, Type {}, Quantity {}, Price {}",
                order.order_id(), order.type(), order.quantity(), order.price());

    // TODO: Implement order processing logic
    // For example:
    // - Validate the order
    // - Apply any business rules
    // - Update order status

    // For this example, we'll just set the status to ACCEPTED
    response.set_status(cs_proto::OrderStatus::ACCEPTED);

    // Send order to matching engine
    send_order_to_matching(order);

    return response;
}

void OrderProcessor::send_order_to_matching(const cs_proto::FuturesOrder& order) {
    kafka_manager_.produce("matching_orders_topic", order, order.client_id());
    Logger::log(INFO, "Order sent to matching engine: Order ID {}, Client ID {}", order.order_id(), order.client_id());
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