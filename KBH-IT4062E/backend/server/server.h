#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <jsoncpp/json/json.h>
#include "room_manager.h"
#include "../database/database.h"
#include "../typing_engine/typing_engine.h"

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
    void on_sign_in(int fd, const Json::Value& msg);
    void on_create_account(int fd, const Json::Value& msg);
    void on_change_password(int fd, const Json::Value& msg);
    void on_sign_out(int fd);
    void on_create_room(int fd);
    void on_join_room(int fd, const Json::Value& msg);
    void on_join_random(int fd);
    void on_exit_room(int fd);
    void on_ready(int fd);
    void on_unready(int fd);
    void on_set_private(int fd, const Json::Value& msg);
    void on_start_game(int fd, const Json::Value& msg);
    void on_start_training(int fd);
    void on_save_training_result(int fd, const Json::Value& msg);
    void on_leaderboard(int fd);
    void on_input(int fd, const Json::Value& msg);
    
    // Helper to broadcast room_state
    void broadcast_room_state(Room* room);
    
    // Check if user_id is already logged in
    bool is_user_logged_in(int64_t user_id) const;
    
    // Get server time in ms
    int64_t get_server_time_ms() const;
    
    // Training session tracking
    struct PlayerMetrics {
        int word_idx = 0;
        int64_t latest_time_ms = 0;
        double progress = 0.0;
        double wpm = 0.0;
        double accuracy = 0.0;
        int total_correct_chars = 0;
        int total_chars_typed = 0;
    };
    
    struct TrainingSession {
        std::string paragraph;
        std::vector<std::string> paragraph_words;
        int total_words;
        int64_t start_time_ms;
        int duration_ms;
        std::string display_name;
        PlayerMetrics metrics;  // Store metrics directly instead of using TypingEngine
    };
    std::unordered_map<int, TrainingSession> training_sessions_;
    
    // Client tracking
    struct ClientInfo {
        int client_id;
        std::string display_name;
        std::string recv_buffer; // for TCP streaming
        int64_t user_id = -1;    // -1 = guest, positive = authenticated user
        std::string username;    // empty for guests
    };
    
    std::unordered_map<int, ClientInfo> clients_;
    int next_client_id_ = 1;

    std::string ip_;
    int port_;
    int server_fd_;
    RoomManager room_manager_;
    std::chrono::steady_clock::time_point start_time_;
};
