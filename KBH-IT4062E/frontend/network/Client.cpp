#include "Client.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

Client::Client() : sockfd_(-1) {}
Client::~Client() { close(); }

bool Client::connectTo(const std::string& ip, int port) {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        perror("inet_pton");
        return false;
    }

    if (connect(sockfd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return false;
    }

    std::cout << "[Client] Connected\n";
    return true;
}

bool Client::sendLine(const std::string& line) {
    if (sockfd_ < 0) return false;
    std::string msg = line;
    if (msg.back() != '\n') msg += '\n';

    ssize_t n = send(sockfd_, msg.c_str(), msg.size(), 0);
    return n == (ssize_t)msg.size();
}

bool Client::receiveLine(std::string& out) {
    if (sockfd_ < 0) return false;

    char buf[512];
    ssize_t n = recv(sockfd_, buf, sizeof(buf), 0);
    if (n <= 0) return false;

    recv_buffer_.append(buf, n);

    auto pos = recv_buffer_.find('\n');
    if (pos == std::string::npos) return false;

    out = recv_buffer_.substr(0, pos + 1);
    recv_buffer_.erase(0, pos + 1);
    return true;
}

void Client::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}
