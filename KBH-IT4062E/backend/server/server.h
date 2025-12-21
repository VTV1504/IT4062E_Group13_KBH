#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

#include "room_manager.h"
#include "database.h"

class Server {
public:
    Server(const std::string& ip, int port, Database* db);
    ~Server();

    void start();

private:
    void handle_message(int fd, const std::string& msg);
    void send_to_room(Room* room, const std::string& msg, int except_fd = -1);

    std::string ip_;
    int port_;
    int server_fd_;

    RoomManager room_manager_;
    std::vector<int> client_fds_;

    std::unordered_map<int, std::string> fd_to_player_;
    std::unordered_map<int, std::chrono::steady_clock::time_point> start_time_;
};

#endif
