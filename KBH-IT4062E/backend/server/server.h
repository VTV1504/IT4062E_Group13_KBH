#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include "room_manager.h"
#include "database.h"

class Server {
public:
    Server(const std::string& ip, int port, Database* db);
    void start();

private:
    void handle_message(int fd, const std::string& msg);
    void handle_disconnect(int fd);

    // helper: send to all players in a room
    void broadcast(Room* room, const std::string& msg);

    std::string ip_;
    int port_;
    int server_fd_;
    Database* db_;
    RoomManager room_manager_;
    std::unordered_map<int, std::string> recv_buffers_;
    std::unordered_map<int, std::string> client_names_;
    std::unordered_set<int> clients_;
};
