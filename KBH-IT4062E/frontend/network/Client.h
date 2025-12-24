#pragma once
#include <string>

class Client {
public:
    Client();
    ~Client();

    bool connectTo(const std::string& ip, int port);
    bool sendLine(const std::string& line);
    bool receiveLine(std::string& out);

    int fd() const { return sockfd_; }
    void close();

private:
    int sockfd_;
    std::string recv_buffer_;
};
