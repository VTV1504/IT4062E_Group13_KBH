#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

// TODO: Constructor và Destructor
Server::Server(const std::string& socket_path) : socket_path_(socket_path), server_fd_(-1) {}

Server::~Server() {
    // TODO: Đóng socket khi server bị hủy
    if (server_fd_ != -1) {
        close(server_fd_);
    }
}

// TODO: Hàm start() để lắng nghe kết nối
void Server::start() {
    struct sockaddr_un addr;

    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        perror("Socket creation failed");
        return;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path_.c_str());

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("Bind failed");
        return;
    }

    if (listen(server_fd_, 5) == -1) {
        perror("Listen failed");
        return;
    }

    std::cout << "Server is listening on " << socket_path_ << "\n";
    while (true) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        // TODO: Xử lý client kết nối
        handle_client(client_fd);
    }
}

void Server::handle_client(int client_fd) {
    // TODO: Phân tích và xử lý yêu cầu của client
    std::cout << "Handling client with fd: " << client_fd << "\n";
    close(client_fd);
}
