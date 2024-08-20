/*************************************************************************
 * @file    user.h
 * @brief   Define user basic info
 * @author  stanjiang
 * @date    2024-08-17
 * @copyright
***/

#ifndef _ORDER_SERVER_USER_H_
#define _ORDER_SERVER_USER_H_

#include <string>
#include <vector>
#include "wallet.h"

class User {
public:
    User(uint64_t id, const std::string& username);
    ~User();

    // Getters
    uint64_t getId() const;
    const std::string& getUsername() const;
    const Wallet& getWallet() const;

    // Profile management
    void updateProfile(const std::string& newInfo);
    const std::string& getProfile() const;

    // Trading related methods
    bool canTrade() const;
    void setTradeStatus(bool status);

private:
    uint64_t id_;
    std::string username_;
    std::string profile_;
    bool canTrade_;
    Wallet wallet_;
};

#endif // _ORDER_SERVER_USER_H_