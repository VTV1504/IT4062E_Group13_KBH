#pragma once

#include <string>
#include "room_manager.h"
#include "database.h"
#include <unordered_map>

class Server {
public:
    Server(const std::string& ip, int port, Database* db);
    void start();

private:
    void handle_client(int client_fd);
    void handle_message(int fd, const std::string& msg);

    // helper: send to all players in a room
    void broadcast(Room* room, const std::string& msg);

    std::unordered_map<int, std::string> fd_to_username_;

    std::string ip_;
    int port_;
    int server_fd_;
    RoomManager room_manager_;
};
