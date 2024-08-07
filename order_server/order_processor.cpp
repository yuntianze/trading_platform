#include "order_processor.h"
#include "logger.h"

OrderProcessor::OrderProcessor() : kafka_manager_(KafkaManager::instance()) {
}

OrderProcessor::~OrderProcessor() {
}

int OrderProcessor::init() {
    // Perform any necessary initialization
    LOG(INFO, "OrderProcessor initialized");
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
    LOG(INFO, "Processing order: ID {}, Type {}, Quantity {}, Price {}",
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
    LOG(INFO, "Order sent to matching engine: Order ID {}, Client ID {}", order.order_id(), order.client_id());
}

void OrderProcessor::match_orders() {
    // TODO: Implement order matching logic
    // This could involve:
    // - Comparing buy and sell orders
    // - Executing trades when orders match
    // - Updating order statuses
    // - Sending trade confirmations

    // LOG(INFO, "Order matching completed");
}

cspkg::AccountLoginRes OrderProcessor::validate_login(const cspkg::AccountLoginReq& login_req) {
    cspkg::AccountLoginRes response;
    response.set_account(login_req.account());
    response.set_result(0);  // Login successful
    LOG(INFO, "Login successful for account {}", login_req.account());

    // std::lock_guard<std::mutex> lock(session_mutex_);

    // // Check if the account exists and the session key is valid
    // auto it = user_sessions_.find(login_req.account());
    // if (it != user_sessions_.end() && it->second == login_req.session_key()) {
    //     response.set_result(0);  // Login successful
    //     LOG(INFO, "Login successful for account {}", login_req.account());
    // } else {
    //     response.set_result(1);  // Login failed
    //     LOG(ERROR, "Login failed for account {}", login_req.account());
    // }

    return response;
}

void OrderProcessor::allocate_user_object(uint32_t account) {
    std::lock_guard<std::mutex> lock(session_mutex_);

    // Generate a new session key (this is a simplified example)
    std::string new_session_key = std::to_string(std::rand());

    // Store the new session
    user_sessions_[account] = new_session_key;

    LOG(INFO, "User object allocated for account {}", account);
}