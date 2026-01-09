#pragma once

#include <string>
#include <functional>
#include <jsoncpp/json/json.h>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool connectTo(const std::string& host, int port);
    void disconnect();
    bool isConnected() const { return sockfd_ >= 0; }

    bool sendCommand(const std::string& type, const Json::Value& payload);
    void poll();

    void setOnMessage(std::function<void(const Json::Value&)> cb) { onMessage_ = std::move(cb); }
    const std::string& lastError() const { return last_error_; }

private:
    int sockfd_ = -1;
    std::string buffer_;
    std::function<void(const Json::Value&)> onMessage_;
    std::string last_error_;
};
