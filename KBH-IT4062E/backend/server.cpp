#include "server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

Server::Server(const std::string& ip, int port, Database* db)
    : ip_(ip),
      port_(port),
      server_fd_(-1),
      room_manager_(db) {}

Server::~Server() {
    for (int fd : client_fds_) close(fd);
    if (server_fd_ != -1) close(server_fd_);
}

void Server::send_to_room(Room* room, const std::string& msg, int except_fd) {
    if (!room) return;
    for (const auto& p : room->players()) {
        if (p.fd == except_fd) continue;
        send(p.fd, msg.c_str(), msg.size(), 0);
    }
}


void Server::handle_message(int fd, const std::string& rawMsg) {
    std::string msg = rawMsg;
    msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());
    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());

    std::istringstream iss(msg);
    std::string cmd;
    iss >> cmd;

    // =========================
    // JOIN <name> <room_id>
    // =========================
    if (cmd == "JOIN") {
        std::string name, room_id;
        iss >> name >> room_id;

        bool is_host = false;
        std::string err;
        Room* room = room_manager_.join_room(room_id, name, fd, is_host, err);

        if (!room) {
            if (err == "ROOM_FULL") {
                send(fd, "ROOM_FULL\n", 10, 0);
            } else if (err == "NAME_TAKEN") {
                send(fd, "NAME_TAKEN\n", 11, 0);
            } else {
                send(fd, "JOIN_ERROR\n", 11, 0);
            }
            return;
        }

        fd_to_player_[fd] = name;

        if (is_host) {
            std::string msg = "HOST " + room_id + "\n";
            send(fd, msg.c_str(), msg.size(), 0);
        } else {
            std::string msg = "JOIN_OK " + room_id + "\n";
            send(fd, msg.c_str(), msg.size(), 0);
        }

        std::string wait_msg = "WAIT_READY 0/" +
                               std::to_string(room->player_count()) +
                               "\n";
        send_to_room(room, wait_msg);
        return;
    }

    // Từ đây trở đi cần biết player đang ở room nào
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        send(fd, "NO_ROOM\n", 8, 0);
        return;
    }

    std::string player_name = fd_to_player_[fd];
    ArenaMode* arena = room->arena();

    // =========================
    // READY
    // =========================
    if (cmd == "READY") {
        arena->set_ready(player_name);

        int ready = arena->ready_count();
        int total = room->player_count();

        std::string state = "WAIT_READY " +
                            std::to_string(ready) + "/" +
                            std::to_string(total) + "\n";
        send_to_room(room, state);
        return;
    }

    // =========================
    // START  (chỉ host được phép)
    // =========================
    if (cmd == "START") {
        if (!room->is_host(fd)) {
            send(fd, "NOT_HOST\n", 9, 0);
            return;
        }

        if (!arena->all_ready()) {
            int ready = arena->ready_count();
            int total = room->player_count();
            std::string msg = "NOT_ALL_READY " +
                              std::to_string(ready) + "/" +
                              std::to_string(total) + "\n";
            send(fd, msg.c_str(), msg.size(), 0);
            return;
        }

        // bắt đầu game: lưu start_time cho tất cả player trong room
        for (const auto& p : room->players()) {
            start_time_[p.fd] = std::chrono::steady_clock::now();
        }

        room->mark_started();

        std::string text = arena->get_target_text();
        std::string start_msg = "GAME_START " + text + "\n";
        send_to_room(room, start_msg);
        return;
    }

    // =========================
    // Nếu không phải JOIN / READY / START → coi là typed_text
    // (chỉ khi game đã START)
    // =========================
    if (!room->has_started()) {
        send(fd, "GAME_NOT_STARTED\n", 17, 0);
        return;
    }

    auto t_start = start_time_[fd];
    auto t_end   = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();

    arena->process_player_result(player_name, msg, elapsed);

    if (arena->all_players_finished()) {
        std::string ranking = "RANKING\n" +
                              arena->get_results_text() +
                              ".\n";
        send_to_room(room, ranking);
    }
}



void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);

    bind(server_fd_, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd_, 10);

    std::cout << "Server running at " << ip_ << ":" << port_ << "\n";

    fd_set master, readset;
    FD_ZERO(&master);
    FD_SET(server_fd_, &master);
    int fd_max = server_fd_;

    while (true) {
        readset = master;
        select(fd_max + 1, &readset, NULL, NULL, NULL);

        for (int fd = 0; fd <= fd_max; fd++) {
            if (!FD_ISSET(fd, &readset)) continue;

            if (fd == server_fd_) {
                int client_fd = accept(server_fd_, nullptr, nullptr);
                client_fds_.push_back(client_fd);
                FD_SET(client_fd, &master);
                if (client_fd > fd_max) fd_max = client_fd;

                send(client_fd, "WELCOME TO ARENA\n", 18, 0);
            } else {
                char buf[4096];
                int bytes = recv(fd, buf, sizeof(buf) - 1, 0);

                if (bytes <= 0) {
                    close(fd);
                    FD_CLR(fd, &master);

                    client_fds_.erase(std::remove(client_fds_.begin(),
                                                  client_fds_.end(), fd),
                                      client_fds_.end());

                    fd_to_player_.erase(fd);
                    start_time_.erase(fd);
                    room_manager_.remove_fd(fd);
                    continue;
                }

                buf[bytes] = '\0';
                handle_message(fd, std::string(buf));
            }
        }
    }
}

