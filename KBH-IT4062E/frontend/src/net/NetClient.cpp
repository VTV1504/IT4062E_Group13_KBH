#include "NetClient.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <iostream>

NetClient::NetClient() {}

NetClient::~NetClient() {
    disconnect();
}

bool NetClient::connect(const std::string& ip, int port) {
    if (connected_) return false;
    
    // Make sure no thread is running from a previous connection attempt
    if (recv_thread_.joinable()) {
        std::cerr << "[NetClient] Warning: receiver thread still running, joining first\n";
        should_stop_ = true;
        recv_thread_.join();
    }
    
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "[NetClient] Failed to create socket: " << strerror(errno) << "\n";
        return false;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "[NetClient] Invalid address: " << ip << "\n";
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    std::cout << "[NetClient] Attempting to connect to " << ip << ":" << port << "...\n";
    
    if (::connect(sockfd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[NetClient] Failed to connect to " << ip << ":" << port 
                  << " - " << strerror(errno) << "\n";
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    connected_ = true;
    should_stop_ = false;
    recv_buffer_.clear();
    
    std::cout << "[NetClient] Connected to " << ip << ":" << port << "\n";
    
    // Start receive thread
    try {
        recv_thread_ = std::thread(&NetClient::receive_thread, this);
        std::cout << "[NetClient] Receiver thread created successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "[NetClient] Failed to create receiver thread: " << e.what() << "\n";
        connected_ = false;
        close(sockfd_);
        sockfd_ = -1;
        return false;
    }
    
    return true;
}

void NetClient::disconnect() {
    if (!connected_) return;
    
    should_stop_ = true;
    connected_ = false;
    
    if (sockfd_ >= 0) {
        close(sockfd_);
        sockfd_ = -1;
    }
    
    if (recv_thread_.joinable()) {
        recv_thread_.join();
    }
    
    std::cout << "[NetClient] Disconnected\n";
}

void NetClient::receive_thread() {
    char buffer[4096];
    
    // std::cout << "[NetClient] Receiver thread started\n";
    
    try {
        while (!should_stop_ && connected_) {
            int n = recv(sockfd_, buffer, sizeof(buffer) - 1, 0);
            
            if (n <= 0) {
                std::cerr << "[NetClient] Connection closed by server (recv=" << n << ")\n";
                connected_ = false;
                break;
            }
            
            buffer[n] = '\0';
            recv_buffer_.append(buffer, n);
            
            // Process complete JSON lines
            size_t pos;
            while ((pos = recv_buffer_.find('\n')) != std::string::npos) {
                std::string line = recv_buffer_.substr(0, pos);
                recv_buffer_.erase(0, pos + 1);
                
                // Trim whitespace
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                    line.pop_back();
                }
                
                if (line.empty()) continue;
                
                // std::cout << "[NetClient] Received: " << line << "\n";
                
                // Parse JSON
                Json::Value msg;
                Json::CharReaderBuilder builder;
                std::istringstream ss(line);
                std::string errs;
                
                if (!Json::parseFromStream(builder, ss, &msg, &errs)) {
                    std::cerr << "[NetClient] JSON parse error: " << errs << "\n";
                    continue;
                }
                
                // Parse event and push to queue
                auto event = parse_event(msg);
                if (event) {
                    // std::cout << "[NetClient] Parsed event type: " << (int)event->type << "\n";
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    event_queue_.push(std::move(event));
                } else {
                    std::cerr << "[NetClient] Failed to parse event from JSON\n";
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[NetClient] Exception in receiver thread: " << e.what() << "\n";
        connected_ = false;
    } catch (...) {
        std::cerr << "[NetClient] Unknown exception in receiver thread\n";
        connected_ = false;
    }
    
    std::cout << "[NetClient] Receiver thread stopped\n";
}

std::unique_ptr<NetEvent> NetClient::parse_event(const Json::Value& json) {
    try {
        if (!json.isMember("type") || !json["type"].isString()) {
            std::cerr << "[NetClient] Event missing 'type' field or type is not a string\n";
            std::cerr << "[NetClient] JSON: " << json.toStyledString() << "\n";
            return nullptr;
        }
        
        std::string type = json["type"].asString();
        // std::cout << "[NetClient] Parsing event type: " << type << "\n";
        
        if (type == "hello") {
            auto evt = std::make_unique<HelloEvent>();
            if (json.isMember("client_id")) evt->client_id = json["client_id"].asInt();
            if (json.isMember("server_time_ms")) evt->server_time_ms = json["server_time_ms"].asInt64();
            // std::cout << "[NetClient] Parsed hello: client_id=" << evt->client_id << "\n";
            return evt;
        }
        
        if (type == "time_sync") {
            auto evt = std::make_unique<TimeSyncEvent>();
            if (json.isMember("client_id")) evt->client_id = json["client_id"].asInt();
            if (json.isMember("server_time_ms")) evt->server_time_ms = json["server_time_ms"].asInt64();
            if (json.isMember("client_time_ms")) evt->client_time_ms = json["client_time_ms"].asInt64();
            return evt;
        }
        
        if (type == "sign_in_response") {
            auto evt = std::make_unique<SignInResponseEvent>();
            if (json.isMember("success")) evt->success = json["success"].asBool();
            if (json.isMember("user_id")) evt->user_id = json["user_id"].asInt64();
            if (json.isMember("username")) evt->username = json["username"].asString();
            if (json.isMember("error")) evt->error_message = json["error"].asString();
            else if (json.isMember("message")) evt->error_message = json["message"].asString();
            return evt;
        }
        
        if (type == "create_account_response") {
            auto evt = std::make_unique<CreateAccountResponseEvent>();
            if (json.isMember("success")) evt->success = json["success"].asBool();
            if (json.isMember("user_id")) evt->user_id = json["user_id"].asInt64();
            if (json.isMember("username")) evt->username = json["username"].asString();
            if (json.isMember("error")) evt->error_message = json["error"].asString();
            else if (json.isMember("message")) evt->error_message = json["message"].asString();
            return evt;
        }
        
        if (type == "change_password_response") {
            auto evt = std::make_unique<ChangePasswordResponseEvent>();
            if (json.isMember("success")) evt->success = json["success"].asBool();
            // Read both "error" and "message" for backward compatibility
            if (json.isMember("error")) {
                evt->error_message = json["error"].asString();
            } else if (json.isMember("message")) {
                evt->error_message = json["message"].asString();
            }
            return evt;
        }
    
    if (type == "room_state") {
        auto evt = std::make_unique<RoomStateEvent>();
        if (json.isMember("room_id")) evt->room_id = json["room_id"].asString();
        if (json.isMember("is_private")) evt->is_private = json["is_private"].asBool();
        if (json.isMember("max_players")) evt->max_players = json["max_players"].asInt();
        if (json.isMember("self_client_id")) evt->self_client_id = json["self_client_id"].asInt();
        if (json.isMember("host_slot_idx")) evt->host_slot_idx = json["host_slot_idx"].asInt();
        if (json.isMember("all_ready")) evt->all_ready = json["all_ready"].asBool();
        if (json.isMember("can_start")) evt->can_start = json["can_start"].asBool();
        
        if (json.isMember("slots") && json["slots"].isArray()) {
            for (int i = 0; i < 8 && i < (int)json["slots"].size(); i++) {
                const auto& slot = json["slots"][i];
                evt->slots[i].slot_idx = i;
                
                if (slot.isMember("occupied")) evt->slots[i].occupied = slot["occupied"].asBool();
                if (evt->slots[i].occupied) {
                    if (slot.isMember("client_id")) evt->slots[i].client_id = slot["client_id"].asInt();
                    if (slot.isMember("display_name")) evt->slots[i].display_name = slot["display_name"].asString();
                    if (slot.isMember("is_host")) evt->slots[i].is_host = slot["is_host"].asBool();
                    if (slot.isMember("is_ready")) evt->slots[i].is_ready = slot["is_ready"].asBool();
                    if (slot.isMember("knight_idx")) evt->slots[i].knight_idx = slot["knight_idx"].asInt();
                }
            }
        }
        return evt;
    }
    
    if (type == "game_init") {
        auto evt = std::make_unique<GameInitEvent>();
        if (json.isMember("room_id")) evt->room_id = json["room_id"].asString();
        if (json.isMember("server_start_ms")) evt->server_start_ms = json["server_start_ms"].asInt64();
        if (json.isMember("duration_ms")) evt->duration_ms = json["duration_ms"].asInt();
        if (json.isMember("paragraph")) evt->paragraph = json["paragraph"].asString();
        if (json.isMember("total_words")) evt->total_words = json["total_words"].asInt();
        
        if (json.isMember("players") && json["players"].isArray()) {
            for (const auto& p : json["players"]) {
                GamePlayerData player;
                if (p.isMember("slot_idx")) player.slot_idx = p["slot_idx"].asInt();
                if (p.isMember("client_id")) player.client_id = p["client_id"].asInt();
                if (p.isMember("display_name")) player.display_name = p["display_name"].asString();
                evt->players.push_back(player);
            }
        }
        return evt;
    }
    
    if (type == "game_state") {
        auto evt = std::make_unique<GameStateEvent>();
        if (json.isMember("room_id")) evt->room_id = json["room_id"].asString();
        if (json.isMember("server_now_ms")) evt->server_now_ms = json["server_now_ms"].asInt64();
        if (json.isMember("duration_ms")) evt->duration_ms = json["duration_ms"].asInt();
        if (json.isMember("ended")) evt->ended = json["ended"].asBool();
        
        if (json.isMember("players") && json["players"].isArray()) {
            for (int i = 0; i < 8 && i < (int)json["players"].size(); i++) {
                const auto& p = json["players"][i];
                evt->players[i].slot_idx = i;
                
                if (p.isMember("occupied")) evt->players[i].occupied = p["occupied"].asBool();
                if (evt->players[i].occupied) {
                    if (p.isMember("word_idx")) evt->players[i].word_idx = p["word_idx"].asInt();
                    if (p.isMember("latest_time_ms")) evt->players[i].latest_time_ms = p["latest_time_ms"].asInt64();
                    if (p.isMember("progress")) evt->players[i].progress = p["progress"].asDouble();
                    if (p.isMember("wpm")) evt->players[i].wpm = p["wpm"].asDouble();
                    if (p.isMember("accuracy")) evt->players[i].accuracy = p["accuracy"].asDouble();
                }
            }
        }
        return evt;
    }
    
    if (type == "game_end") {
        auto evt = std::make_unique<GameEndEvent>();
        if (json.isMember("room_id")) evt->room_id = json["room_id"].asString();
        if (json.isMember("reason")) evt->reason = json["reason"].asString();
        
        if (json.isMember("rankings") && json["rankings"].isArray()) {
            for (const auto& r : json["rankings"]) {
                RankingData rank;
                if (r.isMember("rank")) rank.rank = r["rank"].asInt();
                if (r.isMember("slot_idx")) rank.slot_idx = r["slot_idx"].asInt();
                if (r.isMember("client_id")) rank.client_id = r["client_id"].asInt();
                if (r.isMember("display_name")) rank.display_name = r["display_name"].asString();
                if (r.isMember("word_idx")) rank.word_idx = r["word_idx"].asInt();
                if (r.isMember("latest_time_ms")) rank.latest_time_ms = r["latest_time_ms"].asInt64();
                if (r.isMember("wpm")) rank.wpm = r["wpm"].asDouble();
                if (r.isMember("accuracy")) rank.accuracy = r["accuracy"].asDouble();
                evt->rankings.push_back(rank);
            }
        }
        return evt;
    }
    
    if (type == "error") {
        auto evt = std::make_unique<ErrorEvent>();
        if (json.isMember("code")) evt->code = json["code"].asString();
        if (json.isMember("message")) evt->message = json["message"].asString();
        return evt;
    }
    
    if (type == "info") {
        auto evt = std::make_unique<InfoEvent>();
        if (json.isMember("code")) evt->code = json["code"].asString();
        if (json.isMember("message")) evt->message = json["message"].asString();
        return evt;
    }
    
    std::cerr << "[NetClient] Unknown event type: " << type << "\n";
    return nullptr;
    
    } catch (const std::exception& e) {
        std::cerr << "[NetClient] Exception in parse_event: " << e.what() << "\n";
        return nullptr;
    } catch (...) {
        std::cerr << "[NetClient] Unknown exception in parse_event\n";
        return nullptr;
    }
}

std::unique_ptr<NetEvent> NetClient::poll_event() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (event_queue_.empty()) return nullptr;
    
    auto evt = std::move(event_queue_.front());
    event_queue_.pop();
    return evt;
}

bool NetClient::has_events() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return !event_queue_.empty();
}

void NetClient::send_json_internal(const Json::Value& obj) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string msg = Json::writeString(builder, obj) + "\n";
    send(sockfd_, msg.c_str(), msg.size(), 0);
}

void NetClient::send_time_sync(int64_t client_time_ms) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send time_sync.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "time_sync";
    msg["client_time_ms"] = (Json::Int64)client_time_ms;
    send_json_internal(msg);
}

void NetClient::send_set_username(const std::string& username) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send set_username.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "set_username";
    msg["username"] = username;
    send_json_internal(msg);
}

void NetClient::send_sign_in(const std::string& username, const std::string& password) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send sign_in.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "sign_in";
    msg["username"] = username;
    msg["password"] = password;
    send_json_internal(msg);
}

void NetClient::send_create_account(const std::string& username, const std::string& password) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send create_account.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "create_account";
    msg["username"] = username;
    msg["password"] = password;
    send_json_internal(msg);
}

void NetClient::send_change_password(const std::string& username, const std::string& old_password, const std::string& new_password) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send change_password.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "change_password";
    msg["username"] = username;
    msg["old_password"] = old_password;
    msg["new_password"] = new_password;
    send_json_internal(msg);
}

void NetClient::send_sign_out() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send sign_out.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "sign_out";
    send_json_internal(msg);
    
    std::cout << "[NetClient] Sent sign_out request\n";
}

void NetClient::send_create_room() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send create_room.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "create_room";
    send_json_internal(msg);
}

void NetClient::send_join_room(const std::string& room_id) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send join_room.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "join_room";
    msg["room_id"] = room_id;
    send_json_internal(msg);
}

void NetClient::send_join_random() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send join_random.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "join_random";
    send_json_internal(msg);
}

void NetClient::send_exit_room() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send exit_room.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "exit_room";
    send_json_internal(msg);
}

void NetClient::send_ready() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send ready.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "ready";
    send_json_internal(msg);
}

void NetClient::send_unready() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send unready.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "unready";
    send_json_internal(msg);
}

void NetClient::send_set_private(bool is_private) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send set_private.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "set_private";
    msg["is_private"] = is_private;
    send_json_internal(msg);
}

void NetClient::send_start_game(int duration_ms) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send start_game.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "start_game";
    msg["duration_ms"] = duration_ms;
    send_json_internal(msg);
}

void NetClient::send_start_training() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send start_training.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "start_training";
    send_json_internal(msg);
}

void NetClient::send_save_training_result(const std::string& paragraph, double wpm, double accuracy,
                                           int duration_ms, int words_committed) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send save_training_result.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "save_training_result";
    msg["paragraph"] = paragraph;
    msg["wpm"] = wpm;
    msg["accuracy"] = accuracy;
    msg["duration_ms"] = duration_ms;
    msg["words_committed"] = words_committed;
    send_json_internal(msg);
}

void NetClient::send_input(const std::string& room_id, int word_idx, const Json::Value& char_events) {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send input.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "input";
    msg["room_id"] = room_id;
    msg["word_idx"] = word_idx;
    msg["char_events"] = char_events;
    send_json_internal(msg);
}

void NetClient::send_leaderboard() {
    if (!connected_) {
        std::cerr << "[NetClient] Error: Not connected to server. Cannot send leaderboard.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(send_mutex_);
    
    Json::Value msg;
    msg["type"] = "leaderboard";
    send_json_internal(msg);
}
