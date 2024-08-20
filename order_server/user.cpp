#include "user.h"

User::User(uint64_t id, const std::string& username)
    : id_(id), username_(username), canTrade_(false), wallet_() {
}

User::~User() {
}

uint64_t User::getId() const {
    return id_;
}

const std::string& User::getUsername() const {
    return username_;
}

const Wallet& User::getWallet() const {
    return wallet_;
}

void User::updateProfile(const std::string& newInfo) {
    profile_ = newInfo;
}

const std::string& User::getProfile() const {
    return profile_;
}

bool User::canTrade() const {
    return canTrade_;
}

void User::setTradeStatus(bool status) {
    canTrade_ = status;
}