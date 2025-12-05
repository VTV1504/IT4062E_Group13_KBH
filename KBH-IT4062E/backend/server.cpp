#include "server.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

Server::Server(const std::string& ip, int port, GameManager* gm)
    : ip_(ip), port_(port), server_fd_(-1), game_manager(gm)
{
    arena = dynamic_cast<ArenaMode*>(game_manager->get_current_mode());
}

Server::~Server() {
    for (int fd : client_fds) close(fd);
    if (server_fd_ != -1) close(server_fd_);
}

void Server::broadcast(const std::string& msg, int except_fd) {
    for (int fd : client_fds) {
        if (fd != except_fd) {
            send(fd, msg.c_str(), msg.size(), 0);
        }
    }
}

void Server::handle_message(int fd, const std::string& rawMsg) {
    std::string msg = rawMsg;
    msg.erase(std::remove(msg.begin(), msg.end(), '\r'), msg.end());
    msg.erase(std::remove(msg.begin(), msg.end(), '\n'), msg.end());

    std::istringstream iss(msg);
    std::string cmd;
    iss >> cmd;

    // -----------------------------
    // JOIN <player_name>
    // -----------------------------
    if (cmd == "JOIN") {
        std::string name;
        iss >> name;

        fd_to_player[fd] = name;
        arena->add_player(name);

        send(fd, "WAITING FOR OTHER PLAYERS...\n", 30, 0);

        // đủ người → START
        if (arena->get_player_count() >= 2) {
            arena->start();

            // chuẩn bị thời gian start cho từng client
            for (int cfd : client_fds) {
                start_time[cfd] = std::chrono::steady_clock::now();
            }

            std::string text = arena->get_target_text();
            broadcast("START " + text + "\n");
        }

        return;
    }

    // -----------------------------
    // bất cứ input nào khác → coi là typed_text
    // -----------------------------
    std::string player = fd_to_player[fd];

    auto t_start = start_time[fd];
    auto t_end   = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();

    arena->process_player_result(player, msg, elapsed);

    if (arena->all_players_finished()) {
        std::string ranking = "RANKING\n" +
                              arena->get_results_text() +
                              ".\n";
        broadcast(ranking);
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
                client_fds.push_back(client_fd);
                FD_SET(client_fd, &master);

                if (client_fd > fd_max) fd_max = client_fd;

                send(client_fd, "WELCOME TO ARENA\n", 18, 0);
            }
            else {
                char buf[4096];
                int bytes = recv(fd, buf, sizeof(buf)-1, 0);

                if (bytes <= 0) {
                    close(fd);
                    FD_CLR(fd, &master);
                    client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), fd), client_fds.end());
                    continue;
                }

                buf[bytes] = '\0';
                handle_message(fd, std::string(buf));
            }
        }
    }
}
