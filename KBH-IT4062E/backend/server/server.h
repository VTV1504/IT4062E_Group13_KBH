#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <memory>

#include "room_manager.h"
#include "database.h"
#include "typing_session.h"
#include <jsoncpp/json/json.h>

struct SelfTrainingSession {
    std::string text;
    std::unique_ptr<TypingSession> session;
    std::chrono::steady_clock::time_point start_time;
    int duration_seconds = 75;
    bool active = false;
};

struct ClientSession {
    int fd = -1;
    std::string buffer;
    bool authenticated = false;
    bool is_guest = true;
    std::string username;
    SelfTrainingSession self_training;
    std::chrono::steady_clock::time_point connected_at;
};

class Server {
public:
    Server(const std::string& ip, int port, Database* db);
    void start();

private:
    void accept_new_client();
    void remove_client(int fd);

    void handle_message(int fd, const std::string& msg);
    void handle_command(int fd, const std::string& type, const Json::Value& payload);

    void send_json(int fd, const Json::Value& payload);
    void broadcast(Room* room, const Json::Value& payload);

    bool validate_password(const std::string& password) const;
    std::string generate_salt() const;
    std::string hash_password(const std::string& password, const std::string& salt) const;
    double current_timestamp_seconds() const;

    std::string ip_;
    int port_;
    int server_fd_;

    RoomManager room_manager_;
    Database* db_;
    std::unordered_map<int, ClientSession> clients_;
    std::chrono::steady_clock::time_point server_start_;
};
