#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <json/json.h>
#include "room_manager.h"
#include "database.h"

class Server {
public:
    Server(const std::string& ip, int port, Database* db);
    void start();

private:
    void handle_client(int client_fd);
    void handle_message(int fd, const Json::Value& msg);
    
    // NDJSON helpers
    void send_json(int fd, const Json::Value& obj);
    void broadcast_json(Room* room, const Json::Value& obj);
    
    // Message handlers
    void on_time_sync(int fd, const Json::Value& msg);
    void on_set_username(int fd, const Json::Value& msg);
    void on_create_room(int fd);
    void on_join_room(int fd, const Json::Value& msg);
    void on_join_random(int fd);
    void on_exit_room(int fd);
    void on_ready(int fd);
    void on_unready(int fd);
    void on_set_private(int fd, const Json::Value& msg);
    void on_start_game(int fd, const Json::Value& msg);
    void on_input(int fd, const Json::Value& msg);
    
    // Helper to broadcast room_state
    void broadcast_room_state(Room* room);
    
    // Get server time in ms
    int64_t get_server_time_ms() const;
    
    // Client tracking
    struct ClientInfo {
        int client_id;
        std::string display_name;
        std::string recv_buffer; // for TCP streaming
    };
    
    std::unordered_map<int, ClientInfo> clients_;
    int next_client_id_ = 1;

    std::string ip_;
    int port_;
    int server_fd_;
    RoomManager room_manager_;
    std::chrono::steady_clock::time_point start_time_;
};
