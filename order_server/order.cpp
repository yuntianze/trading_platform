#include "order.h"

Order::Order(uint64_t orderId, uint64_t userId, const cs_proto::FuturesOrder& futuresOrder)
    : orderId_(orderId), userId_(userId), futuresOrder_(futuresOrder), filledQuantity_(0), averagePrice_(0) {
}

Order::~Order() {
}

uint64_t Order::getOrderId() const {
    return orderId_;
}

uint64_t Order::getUserId() const {
    return userId_;
}

const cs_proto::FuturesOrder& Order::getFuturesOrder() const {
    return futuresOrder_;
}

void Order::updateStatus(cs_proto::OrderStatus newStatus) {
    futuresOrder_.set_status(newStatus);
}

cs_proto::OrderStatus Order::getStatus() const {
    return futuresOrder_.status();
}

void Order::updateFill(double filledQuantity, double averagePrice) {
    filledQuantity_ += filledQuantity;
    averagePrice_ = ((averagePrice_ * (filledQuantity_ - filledQuantity)) + (averagePrice * filledQuantity)) / filledQuantity_;
}

double Order::getFilledQuantity() const {
    return filledQuantity_;
}

double Order::getRemainingQuantity() const {
    return futuresOrder_.quantity() - filledQuantity_;
}

double Order::getAveragePrice() const {
    return averagePrice_;
}