#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <thread>

Server::Server(const std::string& ip, int port, Database* db)
    : ip_(ip),
      port_(port),
      server_fd_(-1),
      room_manager_(db) {}

void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(ip_.c_str());

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return;
    }

    if (listen(server_fd_, 10) < 0) {
        perror("listen");
        return;
    }

    std::cout << "Server running at " << ip_ << ":" << port_ << "\n";

    while (true) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) continue;

        std::thread(&Server::handle_client, this, client_fd).detach();
    }
}

void Server::handle_client(int client_fd) {
    char buffer[2048];

    while (true) {
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            room_manager_.remove_fd(client_fd);
            close(client_fd);
            return;
        }

        buffer[n] = '\0';
        std::string msg(buffer);

        // (optional) trim trailing \r\n
        while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
            msg.pop_back();
        }

        handle_message(client_fd, msg);
    }
}

void Server::broadcast(Room* room, const std::string& msg) {
    if (!room) return;
    for (const auto& p : room->players()) {
        send(p.fd, msg.c_str(), msg.size(), 0);
    }
}

void Server::handle_message(int fd, const std::string& msg) {
    std::istringstream iss(msg);
    std::string cmd;
    iss >> cmd;

    /* =======================
       JOIN <name> <room_id>
       ======================= */
    if (cmd == "JOIN") {
        std::string name, room_id;
        iss >> name >> room_id;

        bool is_host = false;
        std::string err;
        Room* room = room_manager_.join_room(room_id, name, fd, is_host, err);

        if (!room) {
            std::string out = err.empty() ? "JOIN_FAILED\n" : (err + "\n");
            send(fd, out.c_str(), out.size(), 0);
            return;
        }

        std::string out = "JOIN_OK\n";
        if (is_host) out += "HOST\n";
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        std::string out = "NOT_IN_ROOM\n";
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    ArenaMode* arena = room->arena();
    if (!arena) {
        std::string out = "NO_ARENA\n";
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    /* =======================
       READY
       ======================= */
    if (cmd == "READY") {
        arena->set_ready(fd);
        std::string out = "READY_OK\n";
        send(fd, out.c_str(), out.size(), 0);

        // Không auto-start để tránh mơ hồ.
        // Host phải gửi START khi all_ready.
        if (arena->all_ready()) {
            std::string hint = "ALL_READY\nHOST_CAN_START\n";
            broadcast(room, hint);
        }
        return;
    }

    /* =======================
       START   (only host)
       ======================= */
    if (cmd == "START") {
        if (!room->is_host(fd)) {
            std::string out = "ONLY_HOST_CAN_START\n";
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        if (!arena->all_ready()) {
            std::string out = "NOT_ALL_READY\n";
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        if (room->has_started()) {
            std::string out = "ALREADY_STARTED\n";
            send(fd, out.c_str(), out.size(), 0);
            return;
        }

        room->mark_started();
        arena->start();

        std::string start_msg = "GAME_START\n" + arena->get_text() + "\n";
        broadcast(room, start_msg);
        return;
    }

    /* =======================
       INPUT <full_text>
       (tạm thời vẫn full-string + Enter)
       ======================= */
    if (cmd == "INPUT") {
        std::string content;
        std::getline(iss, content);
        if (!content.empty() && content[0] == ' ')
            content.erase(0, 1);

        arena->process_player_input(fd, content);

        if (arena->finished()) {
            std::string result = "GAME_END\n" + arena->get_ranking();
            broadcast(room, result);
            room->mark_ended();
        }
        return;
    }

    std::string out = "UNKNOWN_COMMAND\n";
    send(fd, out.c_str(), out.size(), 0);
}
