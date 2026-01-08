#pragma once
#include <string>
#include <functional>
#include "Models.h"

class Session {
public:
    bool isLoggedIn() const { return loggedIn; }
    const UserInfo& user() const { return current; }

    void signIn(const std::string& username);
    void signOut();

    void setOnAuthChanged(std::function<void()> cb) { onAuthChanged = std::move(cb); }

private:
    bool loggedIn = false;
    UserInfo current{};
    std::function<void()> onAuthChanged;
};
