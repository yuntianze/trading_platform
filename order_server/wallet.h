/*************************************************************************
 * @file    wallet.h
 * @brief   Define user wallet basic info
 * @author  stanjiang
 * @date    2024-08-17
 * @copyright
***/

#ifndef _ORDER_SERVER_WALLET_H_
#define _ORDER_SERVER_WALLET_H_

#include <unordered_map>
#include <string>

class Wallet {
public:
    Wallet();
    ~Wallet();

    // Balance management
    double getColdBalance(const std::string& currency) const;
    double getHotBalance(const std::string& currency) const;
    void depositToColdWallet(const std::string& currency, double amount);
    void depositToHotWallet(const std::string& currency, double amount);
    bool withdrawFromColdWallet(const std::string& currency, double amount);
    bool withdrawFromHotWallet(const std::string& currency, double amount);

    // Transfer between cold and hot wallets
    bool transferFromColdToHot(const std::string& currency, double amount);
    bool transferFromHotToCold(const std::string& currency, double amount);

private:
    std::unordered_map<std::string, double> coldWallet_;
    std::unordered_map<std::string, double> hotWallet_;
};

#endif // _ORDER_SERVER_WALLET_H_