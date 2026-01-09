#include "NetworkClient.h"

#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>

NetworkClient::NetworkClient() = default;

NetworkClient::~NetworkClient() {
    disconnect();
}

bool NetworkClient::connectTo(const std::string& host, int port) {
    if (isConnected()) return true;

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res = nullptr;
    std::string port_str = std::to_string(port);
    int err = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res);
    if (err != 0) {
        last_error_ = "getaddrinfo failed";
        return false;
    }

    int fd = -1;
    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;

        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        ::close(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd < 0) {
        last_error_ = "Unable to connect";
        return false;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    sockfd_ = fd;
    last_error_.clear();
    buffer_.clear();
    return true;
}

void NetworkClient::disconnect() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

bool NetworkClient::sendCommand(const std::string& type, const Json::Value& payload) {
    if (!isConnected()) return false;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string json = Json::writeString(builder, payload);

    std::string message = type + " " + json + "\n";
    ssize_t sent = ::send(sockfd_, message.c_str(), message.size(), 0);
    if (sent < 0) {
        last_error_ = "send failed";
        return false;
    }
    return true;
}

void NetworkClient::poll() {
    if (!isConnected()) return;

    char buf[4096];
    while (true) {
        ssize_t n = ::recv(sockfd_, buf, sizeof(buf) - 1, 0);
        if (n == 0) {
            disconnect();
            last_error_ = "Disconnected";
            return;
        }
        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }
            last_error_ = std::strerror(errno);
            disconnect();
            return;
        }

        buf[n] = '\0';
        buffer_.append(buf);

        size_t pos = 0;
        while ((pos = buffer_.find('\n')) != std::string::npos) {
            std::string line = buffer_.substr(0, pos);
            buffer_.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.empty()) continue;

            Json::CharReaderBuilder reader;
            Json::Value payload;
            std::string errs;
            std::istringstream iss(line);
            if (Json::parseFromStream(reader, iss, &payload, &errs)) {
                if (onMessage_) {
                    onMessage_(payload);
                }
            }
        }
    }
}
