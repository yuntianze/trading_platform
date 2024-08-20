#include "wallet.h"
#include <stdexcept>

Wallet::Wallet() {
}

Wallet::~Wallet() {
}

double Wallet::getColdBalance(const std::string& currency) const {
    auto it = coldWallet_.find(currency);
    return (it != coldWallet_.end()) ? it->second : 0.0;
}

double Wallet::getHotBalance(const std::string& currency) const {
    auto it = hotWallet_.find(currency);
    return (it != hotWallet_.end()) ? it->second : 0.0;
}

void Wallet::depositToColdWallet(const std::string& currency, double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Deposit amount must be positive");
    }
    coldWallet_[currency] += amount;
}

void Wallet::depositToHotWallet(const std::string& currency, double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Deposit amount must be positive");
    }
    hotWallet_[currency] += amount;
}

bool Wallet::withdrawFromColdWallet(const std::string& currency, double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Withdrawal amount must be positive");
    }
    if (coldWallet_[currency] >= amount) {
        coldWallet_[currency] -= amount;
        return true;
    }
    return false;
}

bool Wallet::withdrawFromHotWallet(const std::string& currency, double amount) {
    if (amount < 0) {
        throw std::invalid_argument("Withdrawal amount must be positive");
    }
    if (hotWallet_[currency] >= amount) {
        hotWallet_[currency] -= amount;
        return true;
    }
    return false;
}

bool Wallet::transferFromColdToHot(const std::string& currency, double amount) {
    if (withdrawFromColdWallet(currency, amount)) {
        depositToHotWallet(currency, amount);
        return true;
    }
    return false;
}

bool Wallet::transferFromHotToCold(const std::string& currency, double amount) {
    if (withdrawFromHotWallet(currency, amount)) {
        depositToColdWallet(currency, amount);
        return true;
    }
    return false;
}