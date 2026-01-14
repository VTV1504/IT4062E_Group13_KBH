#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>
#include <jsoncpp/json/json.h>
#include "NetEvents.h"

class NetClient {
public:
    NetClient();
    ~NetClient();
    
    // Delete copy constructor and assignment (not copyable due to thread member)
    NetClient(const NetClient&) = delete;
    NetClient& operator=(const NetClient&) = delete;
    
    // Connect to server
    bool connect(const std::string& ip, int port);
    
    // Disconnect
    void disconnect();
    
    // Check if connected
    bool is_connected() const { return connected_; }
    
    // Send messages (thread-safe)
    void send_time_sync(int64_t client_time_ms);
    void send_set_username(const std::string& username);
    void send_sign_in(const std::string& username, const std::string& password);
    void send_create_account(const std::string& username, const std::string& password);
    void send_change_password(const std::string& username, const std::string& old_password, const std::string& new_password);
    void send_sign_out();
    void send_create_room();
    void send_join_room(const std::string& room_id);
    void send_join_random();
    void send_exit_room();
    void send_ready();
    void send_unready();
    void send_set_private(bool is_private);
    void send_start_game(int duration_ms = 50000);
    void send_start_training();
    void send_save_training_result(const std::string& paragraph, double wpm, double accuracy, 
                                    int duration_ms, int words_committed);
    void send_input(const std::string& room_id, int word_idx, const Json::Value& char_events);
    void send_leaderboard();
    
    // Poll events from queue (call from UI thread)
    std::unique_ptr<NetEvent> poll_event();
    bool has_events() const;

private:
    // Thread function
    void receive_thread();
    
    // Send JSON (internal, assumes mutex is locked)
    void send_json_internal(const Json::Value& obj);
    
    // Parse incoming JSON and create events
    std::unique_ptr<NetEvent> parse_event(const Json::Value& json);
    
    int sockfd_ = -1;
    std::atomic<bool> connected_{false};
    std::atomic<bool> should_stop_{false};
    
    // Receive buffer for TCP streaming
    std::string recv_buffer_;
    
    // Thread
    std::thread recv_thread_;
    
    // Event queue (thread-safe)
    std::queue<std::unique_ptr<NetEvent>> event_queue_;
    mutable std::mutex queue_mutex_;
    
    // Send mutex
    std::mutex send_mutex_;
};
