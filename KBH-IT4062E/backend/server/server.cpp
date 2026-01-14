#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <thread>
#include <cstring>

Server::Server(const std::string& ip, int port, Database* db)
    : ip_(ip),
      port_(port),
      server_fd_(-1),
      room_manager_(db),
      start_time_(std::chrono::steady_clock::now()) {}

void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = inet_addr(ip_.c_str());

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return;
    }

    if (listen(server_fd_, 10) < 0) {
        perror("listen");
        return;
    }

    std::cout << "[SERVER] Running at " << ip_ << ":" << port_ << "\n";

    while (true) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) continue;
        
        // Assign client_id
        ClientInfo info;
        info.client_id = next_client_id_++;
        info.display_name = "Guest " + std::to_string(info.client_id);
        clients_[client_fd] = info;
        
        std::cout << "[SERVER] Client connected: FD=" << client_fd 
                  << " ID=" << info.client_id << "\n";
        
        // Send hello message
        Json::Value hello;
        hello["type"] = "hello";
        hello["client_id"] = info.client_id;
        hello["server_time_ms"] = (Json::Int64)get_server_time_ms();
        send_json(client_fd, hello);

        std::thread(&Server::handle_client, this, client_fd).detach();
    }
}

void Server::handle_client(int client_fd) {
    char buffer[4096];

    while (true) {
        int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            std::cout << "[SERVER] Client disconnected: FD=" << client_fd << "\n";
            
            // Remove from room and broadcast update to remaining players
            Room* room = room_manager_.remove_fd(client_fd);
            if (room) {
                std::cout << "[SERVER] Broadcasting room_state after disconnect\n";
                broadcast_room_state(room);
            }
            
            clients_.erase(client_fd);
            close(client_fd);
            return;
        }

        buffer[n] = '\0';
        
        // Append to recv buffer
        auto& client_info = clients_[client_fd];
        client_info.recv_buffer.append(buffer, n);
        
        // Process complete JSON lines
        size_t pos;
        while ((pos = client_info.recv_buffer.find('\n')) != std::string::npos) {
            std::string line = client_info.recv_buffer.substr(0, pos);
            client_info.recv_buffer.erase(0, pos + 1);
            
            // Trim whitespace
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                line.pop_back();
            }
            
            if (line.empty()) continue;
            
            // Parse JSON
            Json::Value msg;
            Json::CharReaderBuilder builder;
            std::istringstream ss(line);
            std::string errs;
            
            if (!Json::parseFromStream(builder, ss, &msg, &errs)) {
                std::cerr << "[SERVER] JSON parse error: " << errs << "\n";
                continue;
            }
            
            handle_message(client_fd, msg);
        }
    }
}


void Server::handle_message(int fd, const Json::Value& msg) {
    if (!msg.isMember("type") || !msg["type"].isString()) {
        return;
    }
    
    std::string type = msg["type"].asString();
    
    if (type == "time_sync") {
        on_time_sync(fd, msg);
    } else if (type == "set_username") {
        on_set_username(fd, msg);
    } else if (type == "sign_in") {
        on_sign_in(fd, msg);
    } else if (type == "create_account") {
        on_create_account(fd, msg);
    } else if (type == "change_password") {
        on_change_password(fd, msg);
    } else if (type == "sign_out") {
        on_sign_out(fd);
    } else if (type == "create_room") {
        on_create_room(fd);
    } else if (type == "join_room") {
        on_join_room(fd, msg);
    } else if (type == "join_random") {
        on_join_random(fd);
    } else if (type == "exit_room") {
        on_exit_room(fd);
    } else if (type == "ready") {
        on_ready(fd);
    } else if (type == "unready") {
        on_unready(fd);
    } else if (type == "set_private") {
        on_set_private(fd, msg);
    } else if (type == "start_game") {
        on_start_game(fd, msg);
    } else if (type == "start_training") {
        on_start_training(fd);
    } else if (type == "save_training_result") {
        on_save_training_result(fd, msg);
    } else if (type == "input") {
        on_input(fd, msg);
    } else {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "UNKNOWN_TYPE";
        err["message"] = "Unknown message type: " + type;
        send_json(fd, err);
    }
}

// ========== Helper functions ==========

void Server::send_json(int fd, const Json::Value& obj) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string msg = Json::writeString(builder, obj) + "\n";
    send(fd, msg.c_str(), msg.size(), 0);
}

void Server::broadcast_json(Room* room, const Json::Value& obj) {
    if (!room) return;
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string msg = Json::writeString(builder, obj) + "\n";
    
    for (int i = 0; i < 8; i++) {
        const auto& slot = room->get_slot(i);
        if (slot.occupied) {
            send(slot.client_fd, msg.c_str(), msg.size(), 0);
        }
    }
}

int64_t Server::get_server_time_ms() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return elapsed.count();
}

bool Server::is_user_logged_in(int64_t user_id) const {
    if (user_id <= 0) return false;
    
    for (const auto& pair : clients_) {
        if (pair.second.user_id == user_id) {
            return true;
        }
    }
    return false;
}

void Server::broadcast_room_state(Room* room) {
    if (!room) return;
    
    Json::Value state;
    state["type"] = "room_state";
    state["room_id"] = room->id();
    state["is_private"] = room->is_private();
    state["max_players"] = 8;
    state["host_slot_idx"] = room->host_slot_idx();
    state["all_ready"] = room->all_ready();
    state["can_start"] = room->can_start();
    
    Json::Value slots(Json::arrayValue);
    for (int i = 0; i < 8; i++) {
        const auto& slot = room->get_slot(i);
        Json::Value s;
        s["slot_idx"] = i;
        s["occupied"] = slot.occupied;
        
        if (slot.occupied) {
            s["client_id"] = slot.client_id;
            s["display_name"] = slot.display_name;
            s["is_host"] = (i == room->host_slot_idx());
            s["is_ready"] = slot.is_ready;
            s["knight_idx"] = i;
        }
        
        slots.append(s);
    }
    state["slots"] = slots;
    
    // Add self_client_id for each client
    for (int i = 0; i < 8; i++) {
        const auto& slot = room->get_slot(i);
        if (slot.occupied) {
            Json::Value personal_state = state;
            personal_state["self_client_id"] = slot.client_id;
            send_json(slot.client_fd, personal_state);
        }
    }
}

// ========== Message handlers ==========

void Server::on_time_sync(int fd, const Json::Value& msg) {
    Json::Value reply;
    reply["type"] = "time_sync";
    reply["client_id"] = clients_[fd].client_id;
    reply["server_time_ms"] = (Json::Int64)get_server_time_ms();
    
    if (msg.isMember("client_time_ms")) {
        reply["client_time_ms"] = msg["client_time_ms"];
    }
    
    send_json(fd, reply);
}

void Server::on_set_username(int fd, const Json::Value& msg) {
    if (!msg.isMember("username") || !msg["username"].isString()) {
        return;
    }
    
    clients_[fd].display_name = msg["username"].asString();
    
    // If in a room, update slot and broadcast
    Room* room = room_manager_.get_room_of_fd(fd);
    if (room) {
        room->update_display_name(fd, clients_[fd].display_name);
        broadcast_room_state(room);
    }
}

void Server::on_create_room(int fd) {
    if (room_manager_.get_room_of_fd(fd)) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "ALREADY_IN_ROOM";
        err["message"] = "You are already in a room";
        send_json(fd, err);
        return;
    }
    
    Room* room = room_manager_.create_room(fd, clients_[fd].client_id, clients_[fd].display_name);
    
    if (!room) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "CREATE_FAILED";
        err["message"] = "Failed to create room";
        send_json(fd, err);
        return;
    }
    
    broadcast_room_state(room);
}

void Server::on_join_room(int fd, const Json::Value& msg) {
    if (!msg.isMember("room_id") || !msg["room_id"].isString()) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "MISSING_ROOM_ID";
        err["message"] = "room_id is required";
        send_json(fd, err);
        return;
    }
    
    std::string room_id = msg["room_id"].asString();
    std::string err_msg;
    
    Room* room = room_manager_.join_room(room_id, fd, clients_[fd].client_id, 
                                         clients_[fd].display_name, err_msg);
    
    if (!room) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = err_msg.empty() ? "JOIN_FAILED" : err_msg;
        err["message"] = "Room not exsists or is full";
        send_json(fd, err);
        return;
    }
    
    broadcast_room_state(room);
}

void Server::on_join_random(int fd) {
    Room* room = room_manager_.join_random(fd, clients_[fd].client_id, 
                                           clients_[fd].display_name);
    
    if (!room) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NO_AVAILABLE_ROOM";
        err["message"] = "No public room available";
        send_json(fd, err);
        return;
    }
    
    broadcast_room_state(room);
}

void Server::on_exit_room(int fd) {
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        return;
    }
    
    // Remove player and get room pointer (nullptr if room deleted)
    room = room_manager_.remove_fd(fd);
    
    // If room still exists (has players), broadcast new state
    if (room) {
        std::cout << "[SERVER] Broadcasting room_state after exit_room\n";
        broadcast_room_state(room);
    }
}

void Server::on_ready(int fd) {
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NOT_IN_ROOM";
        err["message"] = "You are not in a room";
        send_json(fd, err);
        return;
    }
    
    room->set_ready(fd, true);
    broadcast_room_state(room);
}

void Server::on_unready(int fd) {
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        return;
    }
    
    room->set_ready(fd, false);
    broadcast_room_state(room);
}

void Server::on_set_private(int fd, const Json::Value& msg) {
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NOT_IN_ROOM";
        err["message"] = "You are not in a room";
        send_json(fd, err);
        return;
    }
    
    if (!room->is_host(fd)) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NOT_HOST";
        err["message"] = "Only host can change privacy";
        send_json(fd, err);
        return;
    }
    
    if (!msg.isMember("is_private") || !msg["is_private"].isBool()) {
        return;
    }
    
    room->set_private(msg["is_private"].asBool());
    broadcast_room_state(room);
}

void Server::on_start_game(int fd, const Json::Value& msg) {
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NOT_IN_ROOM";
        err["message"] = "You are not in a room";
        send_json(fd, err);
        return;
    }
    
    if (!room->is_host(fd)) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NOT_HOST";
        err["message"] = "Only host can start game";
        send_json(fd, err);
        return;
    }
    
    if (!room->can_start()) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "CANNOT_START";
        err["message"] = "Cannot start: need at least 2 players, all ready";
        send_json(fd, err);
        return;
    }
    
    int duration_ms = 50000; // default
    if (msg.isMember("duration_ms") && msg["duration_ms"].isInt()) {
        duration_ms = msg["duration_ms"].asInt();
    }
    
    room->start_game(get_server_time_ms(), duration_ms);
    
    // Send game_init
    Json::Value init;
    init["type"] = "game_init";
    init["room_id"] = room->id();
    init["server_start_ms"] = (Json::Int64)room->game_start_time();
    init["duration_ms"] = duration_ms;
    init["paragraph"] = room->paragraph();
    init["total_words"] = room->total_words();
    
    Json::Value players(Json::arrayValue);
    for (int i = 0; i < 8; i++) {
        const auto& slot = room->get_slot(i);
        if (slot.occupied) {
            Json::Value p;
            p["slot_idx"] = i;
            p["client_id"] = slot.client_id;
            p["display_name"] = slot.display_name;
            players.append(p);
        }
    }
    init["players"] = players;
    
    broadcast_json(room, init);
}

void Server::on_input(int fd, const Json::Value& msg) {
    // Check if this is training mode
    auto training_it = training_sessions_.find(fd);
    if (training_it != training_sessions_.end()) {
        // Training mode input handling
        if (!msg.isMember("word_idx") || !msg["word_idx"].isInt() ||
            !msg.isMember("char_events") || !msg["char_events"].isArray()) {
            return;
        }
        
        int word_idx = msg["word_idx"].asInt();
        const Json::Value& char_events = msg["char_events"];
        
        auto& session = training_it->second;
        auto& metrics = session.metrics;
        
        // Update word_idx
        if (word_idx >= metrics.word_idx) {
            metrics.word_idx = word_idx + 1; // Next word index
        }
        
        // Get the target word for accuracy calculation
        std::string target_word = "";
        if (word_idx >= 0 && word_idx < (int)session.paragraph_words.size()) {
            target_word = session.paragraph_words[word_idx];
        }
        
        // Calculate metrics from char_events (same logic as Room)
        int correct_chars = 0;
        int total_chars = 0;
        std::string typed_word = "";
        
        for (const auto& event : char_events) {
            std::string event_type = event["type"].asString();
            
            if (event_type == "char") {
                std::string ch = event["char"].asString();
                if (!ch.empty()) {
                    typed_word += ch[0];
                    total_chars++;
                }
            } else if (event_type == "backspace") {
                if (!typed_word.empty()) {
                    typed_word.pop_back();
                }
            }
            
            if (event.isMember("time_ms")) {
                metrics.latest_time_ms = event["time_ms"].asInt64();
            }
        }
        
        // Calculate correct chars by comparing typed_word with target_word
        size_t min_len = std::min(typed_word.length(), target_word.length());
        for (size_t i = 0; i < min_len; i++) {
            if (typed_word[i] == target_word[i]) {
                correct_chars++;
            }
        }
        
        // Update cumulative stats
        metrics.total_correct_chars += correct_chars;
        metrics.total_chars_typed += total_chars;
        
        // Calculate progress
        metrics.progress = (double)metrics.word_idx / session.total_words;
        
        // Calculate WPM (based on total correct chars typed so far)
        if (metrics.latest_time_ms > session.start_time_ms) {
            double elapsed_ms = metrics.latest_time_ms - session.start_time_ms;
            double elapsed_minutes = elapsed_ms / 60000.0;
            if (elapsed_minutes > 0) {
                metrics.wpm = (metrics.total_correct_chars / 5.0) / elapsed_minutes;
            }
        }
        
        // Calculate accuracy (cumulative)
        if (metrics.total_chars_typed > 0) {
            metrics.accuracy = (metrics.total_correct_chars * 100.0) / metrics.total_chars_typed;
        } else {
            metrics.accuracy = 100.0;
        }
        
        // Send game_state
        Json::Value state;
        state["type"] = "game_state";
        state["room_id"] = "training";
        state["server_now_ms"] = (Json::Int64)get_server_time_ms();
        state["duration_ms"] = session.duration_ms;
        state["ended"] = false;
        
        Json::Value players_arr(Json::arrayValue);
        for (int i = 0; i < 8; i++) {
            Json::Value p;
            p["slot_idx"] = i;
            p["occupied"] = (i == 0);
            
            if (i == 0) {
                p["word_idx"] = metrics.word_idx;
                p["latest_time_ms"] = (Json::Int64)metrics.latest_time_ms;
                p["progress"] = metrics.progress;
                p["wpm"] = metrics.wpm;
                p["accuracy"] = metrics.accuracy;
            }
            
            players_arr.append(p);
        }
        state["players"] = players_arr;
        
        send_json(fd, state);
        
        // Check if game ended (timeout or finished)
        int64_t elapsed = get_server_time_ms() - session.start_time_ms;
        bool timeout = (elapsed >= session.duration_ms);
        bool finished = (metrics.word_idx >= session.total_words);
        
        if (timeout || finished) {
            Json::Value end;
            end["type"] = "game_end";
            end["room_id"] = "training";
            end["reason"] = finished ? "all_finished" : "timeout";
            
            Json::Value ranks(Json::arrayValue);
            Json::Value rank_entry;
            rank_entry["rank"] = 1;
            rank_entry["slot_idx"] = 0;
            rank_entry["client_id"] = fd;
            rank_entry["display_name"] = session.display_name;
            rank_entry["word_idx"] = metrics.word_idx;
            rank_entry["latest_time_ms"] = (Json::Int64)metrics.latest_time_ms;
            rank_entry["wpm"] = metrics.wpm;
            rank_entry["accuracy"] = metrics.accuracy;
            ranks.append(rank_entry);
            end["rankings"] = ranks;
            
            send_json(fd, end);
            
            // Save result to database if user is logged in
            auto client_it = clients_.find(fd);
            if (client_it != clients_.end() && client_it->second.user_id > 0) {
                int64_t user_id = client_it->second.user_id;
                int actual_duration_ms = metrics.latest_time_ms - session.start_time_ms;
                
                std::cout << "[Server] Saving training result for user_id=" << user_id 
                          << " WPM=" << metrics.wpm << " ACC=" << metrics.accuracy << "%\n";
                
                bool saved = room_manager_.db()->save_training_result(
                    user_id,
                    session.paragraph,
                    metrics.wpm,
                    metrics.accuracy,
                    actual_duration_ms,
                    metrics.word_idx
                );
                
                if (saved) {
                    std::cout << "[Server] Training result saved successfully\n";
                } else {
                    std::cout << "[Server] Failed to save training result\n";
                }
            }
            
            // Clean up training session
            training_sessions_.erase(training_it);
        }
        
        return;
    }
    
    // Arena mode input handling
    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room || !room->is_game_started()) {
        return;
    }
    
    if (!msg.isMember("word_idx") || !msg["word_idx"].isInt() ||
        !msg.isMember("char_events") || !msg["char_events"].isArray()) {
        return;
    }
    
    int word_idx = msg["word_idx"].asInt();
    const Json::Value& events = msg["char_events"];
    
    // Process input in typing engine
    room->process_input(fd, word_idx, events);
    
    // Broadcast game_state
    Json::Value state;
    state["type"] = "game_state";
    state["room_id"] = room->id();
    state["server_now_ms"] = (Json::Int64)get_server_time_ms();
    state["duration_ms"] = room->game_duration();
    state["ended"] = false;
    
    Json::Value players_arr(Json::arrayValue);
    for (int i = 0; i < 8; i++) {
        const auto& slot = room->get_slot(i);
        Json::Value p;
        p["slot_idx"] = i;
        p["occupied"] = slot.occupied;
        
        if (slot.occupied) {
            auto metrics = room->get_player_metrics(slot.client_fd);
            p["word_idx"] = metrics.word_idx;
            p["latest_time_ms"] = (Json::Int64)metrics.latest_time_ms;
            p["progress"] = metrics.progress;
            p["wpm"] = metrics.wpm;
            p["accuracy"] = metrics.accuracy;
        }
        
        players_arr.append(p);
    }
    state["players"] = players_arr;
    
    broadcast_json(room, state);
    
    // Check if game ended
    if (room->is_game_ended(get_server_time_ms())) {
        Json::Value end;
        end["type"] = "game_end";
        end["room_id"] = room->id();
        end["reason"] = room->all_finished() ? "all_finished" : "timeout";
        
        auto rankings = room->get_rankings();
        Json::Value ranks(Json::arrayValue);
        for (const auto& r : rankings) {
            Json::Value rank_entry;
            rank_entry["rank"] = r.rank;
            rank_entry["slot_idx"] = r.slot_idx;
            rank_entry["client_id"] = r.client_id;
            rank_entry["display_name"] = r.display_name;
            rank_entry["word_idx"] = r.word_idx;
            rank_entry["latest_time_ms"] = (Json::Int64)r.latest_time_ms;
            rank_entry["wpm"] = r.wpm;
            rank_entry["accuracy"] = r.accuracy;
            ranks.append(rank_entry);
        }
        end["rankings"] = ranks;
        
        broadcast_json(room, end);
        room->end_game();
        
        // Send updated room_state after game ends (all players unready)
        broadcast_room_state(room);
    }
}

void Server::on_start_training(int fd) {
    // Get random paragraph from database
    std::string paragraph = room_manager_.db()->get_random_paragraph("en");
    if (paragraph.empty()) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NO_PARAGRAPH";
        err["message"] = "Failed to get training paragraph";
        send_json(fd, err);
        return;
    }
    
    // Split paragraph into words
    std::vector<std::string> words;
    std::istringstream iss(paragraph);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    int total_words = words.size();
    
    int duration_ms = 300000; // 5 minutes for training
    int64_t start_time = get_server_time_ms();
    
    // Get client display name
    std::string display_name = "Guest";
    auto it = clients_.find(fd);
    if (it != clients_.end() && !it->second.username.empty()) {
        display_name = it->second.username;
    }
    
    // Store training session with empty metrics
    TrainingSession session;
    session.paragraph = paragraph;
    session.paragraph_words = words;
    session.total_words = total_words;
    session.start_time_ms = start_time;
    session.duration_ms = duration_ms;
    session.display_name = display_name;
    session.metrics = PlayerMetrics();  // Initialize with default values
    training_sessions_[fd] = session;
    
    // Send game_init
    Json::Value init;
    init["type"] = "game_init";
    init["room_id"] = "training";
    init["server_start_ms"] = (Json::Int64)start_time;
    init["duration_ms"] = duration_ms;
    init["paragraph"] = paragraph;
    init["total_words"] = total_words;
    
    Json::Value players(Json::arrayValue);
    Json::Value p;
    p["slot_idx"] = 0;
    p["client_id"] = fd;
    p["display_name"] = display_name;
    players.append(p);
    init["players"] = players;
    
    send_json(fd, init);
}

void Server::on_save_training_result(int fd, const Json::Value& msg) {
    // Check if user is logged in
    auto client_it = clients_.find(fd);
    if (client_it == clients_.end() || client_it->second.user_id <= 0) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "NOT_AUTHENTICATED";
        err["message"] = "Must be logged in to save training result";
        send_json(fd, err);
        return;
    }
    
    // Extract training result data from message
    if (!msg.isMember("paragraph") || !msg.isMember("wpm") || 
        !msg.isMember("accuracy") || !msg.isMember("duration_ms") ||
        !msg.isMember("words_committed")) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "MISSING_FIELDS";
        err["message"] = "Missing required fields for save_training_result";
        send_json(fd, err);
        return;
    }
    
    std::string paragraph = msg["paragraph"].asString();
    double wpm = msg["wpm"].asDouble();
    double accuracy = msg["accuracy"].asDouble();
    int duration_ms = msg["duration_ms"].asInt();
    int words_committed = msg["words_committed"].asInt();
    
    int64_t user_id = client_it->second.user_id;
    
    std::cout << "[Server] Saving training result (post-login) for user_id=" << user_id 
              << " WPM=" << wpm << " ACC=" << accuracy << "%\n";
    
    bool saved = room_manager_.db()->save_training_result(
        user_id,
        paragraph,
        wpm,
        accuracy,
        duration_ms,
        words_committed
    );
    
    if (saved) {
        std::cout << "[Server] Training result saved successfully\n";
        Json::Value response;
        response["type"] = "save_result_response";
        response["success"] = true;
        send_json(fd, response);
    } else {
        std::cout << "[Server] Failed to save training result\n";
        Json::Value err;
        err["type"] = "error";
        err["code"] = "SAVE_FAILED";
        err["message"] = "Failed to save training result";
        send_json(fd, err);
    }
}

// ========== Authentication handlers ==========

void Server::on_sign_in(int fd, const Json::Value& msg) {
    if (!msg.isMember("username") || !msg.isMember("password")) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "MISSING_FIELDS";
        err["message"] = "username and password required";
        send_json(fd, err);
        return;
    }
    
    std::string username = msg["username"].asString();
    std::string password = msg["password"].asString();
    
    // Call database function kbh_authenticate
    auto result = room_manager_.db()->authenticate(username, password);
    
    if (result.first == -1) {
        // Authentication failed
        Json::Value err;
        err["type"] = "sign_in_response";
        err["success"] = false;
        err["error"] = "Invalid username or password";
        send_json(fd, err);
        return;
    }
    
    // Check if user is already logged in
    if (is_user_logged_in(result.first)) {
        Json::Value err;
        err["type"] = "sign_in_response";
        err["success"] = false;
        err["error"] = "This account is already logged in";
        send_json(fd, err);
        std::cout << "[SERVER] Sign in rejected for " << username 
                  << " - already logged in\n";
        return;
    }
    
    // Success - update client info
    clients_[fd].user_id = result.first;
    clients_[fd].username = result.second;
    clients_[fd].display_name = result.second;
    
    Json::Value response;
    response["type"] = "sign_in_response";
    response["success"] = true;
    response["user_id"] = (Json::Int64)result.first;
    response["username"] = result.second;
    send_json(fd, response);
    
    std::cout << "[SERVER] Client " << fd << " signed in as " << result.second 
              << " (user_id=" << result.first << ")\n";
}

void Server::on_create_account(int fd, const Json::Value& msg) {
    if (!msg.isMember("username") || !msg.isMember("password")) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "MISSING_FIELDS";
        err["message"] = "username and password required";
        send_json(fd, err);
        return;
    }
    
    std::string username = msg["username"].asString();
    std::string password = msg["password"].asString();
    
    // Call database function kbh_create_user
    int64_t user_id = room_manager_.db()->create_user(username, password);
    
    if (user_id == -1) {
        // Creation failed (likely duplicate username)
        Json::Value err;
        err["type"] = "create_account_response";
        err["success"] = false;
        err["error"] = "Username already exists";
        send_json(fd, err);
        return;
    }
    
    // Success - don't auto-login, user must sign in
    Json::Value response;
    response["type"] = "create_account_response";
    response["success"] = true;
    response["username"] = username;
    send_json(fd, response);
    
    std::cout << "[SERVER] New account created: " << username 
              << " (user_id=" << user_id << ")\n";
}

void Server::on_change_password(int fd, const Json::Value& msg) {
    if (!msg.isMember("username") || !msg.isMember("old_password") || !msg.isMember("new_password")) {
        Json::Value err;
        err["type"] = "error";
        err["code"] = "MISSING_FIELDS";
        err["message"] = "username, old_password, and new_password required";
        send_json(fd, err);
        return;
    }
    
    std::string username = msg["username"].asString();
    std::string old_password = msg["old_password"].asString();
    std::string new_password = msg["new_password"].asString();
    
    // Call database function kbh_change_password
    bool success = room_manager_.db()->change_password(username, old_password, new_password);
    
    Json::Value response;
    response["type"] = "change_password_response";
    response["success"] = success;
    response["error"] = success ? "" : "Invalid old password";
    send_json(fd, response);
    
    std::cout << "[SERVER] Password change for " << username 
              << ": " << (success ? "success" : "failed") << "\n";
}

void Server::on_sign_out(int fd) {
    auto it = clients_.find(fd);
    if (it == clients_.end()) {
        return;
    }
    
    std::cout << "[SERVER] Client " << fd << " signing out";
    if (it->second.user_id > 0) {
        std::cout << " (user_id=" << it->second.user_id << ", username=" << it->second.username << ")";
    }
    std::cout << "\n";
    
    // Clear user authentication
    it->second.user_id = 0;
    it->second.username.clear();
    
    // Send confirmation
    Json::Value response;
    response["type"] = "sign_out_response";
    response["success"] = true;
    send_json(fd, response);
}
