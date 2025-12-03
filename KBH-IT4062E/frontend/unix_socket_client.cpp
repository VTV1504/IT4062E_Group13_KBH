#include "unix_socket_client.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

// TODO: Định nghĩa các phương thức kết nối và giao tiếp với server qua UNIX Socket

void UNIXSocketClient::connect_to_server() {
    struct sockaddr_un addr;
    client_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd_ == -1) {
        perror("Socket creation failed");
        return;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(client_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Connect failed");
        return;
    }

    std::cout << "Connected to server\n";
}

void UNIXSocketClient::send_message(const std::string& message) {
    // TODO: Gửi message qua socket
}

std::string UNIXSocketClient::receive_message() {
    char buffer[256];
    int len = recv(client_fd_, buffer, sizeof(buffer), 0);
    if (len > 0) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
}
