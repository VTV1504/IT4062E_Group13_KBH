#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

#include "game_manager.h"
#include "arena_mode.h"

class Server {
public:
    Server(const std::string& ip, int port, GameManager* gm);
    ~Server();

    void start();

private:
    void broadcast(const std::string& msg, int except_fd = -1);
    void handle_message(int fd, const std::string& msg);

    std::string ip_;
    int port_;
    int server_fd_;

    GameManager* game_manager;
    ArenaMode* arena;

    std::vector<int> client_fds;
    std::unordered_map<int, std::string> fd_to_player;

    // thời gian bắt đầu cho từng client
    std::unordered_map<int, std::chrono::steady_clock::time_point> start_time;
};

#endif
