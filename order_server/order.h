/*************************************************************************
 * @file    order.h
 * @brief   Define order basic info
 * @author  stanjiang
 * @date    2024-08-17
 * @copyright
***/

#ifndef _ORDER_SERVER_ORDER_H_
#define _ORDER_SERVER_ORDER_H_

#include <string>
#include <cstdint>
#include "futures_order.pb.h"

class Order {
public:
    Order(uint64_t orderId, uint64_t userId, const cs_proto::FuturesOrder& futuresOrder);
    ~Order();

    // Getters
    uint64_t getOrderId() const;
    uint64_t getUserId() const;
    const cs_proto::FuturesOrder& getFuturesOrder() const;

    // Order status management
    void updateStatus(cs_proto::OrderStatus newStatus);
    cs_proto::OrderStatus getStatus() const;

    // Fill management
    void updateFill(double filledQuantity, double averagePrice);
    double getFilledQuantity() const;
    double getRemainingQuantity() const;
    double getAveragePrice() const;

private:
    uint64_t orderId_;
    uint64_t userId_;
    cs_proto::FuturesOrder futuresOrder_;
    double filledQuantity_;
    double averagePrice_;
};

#endif // _ORDER_SERVER_ORDER_H_