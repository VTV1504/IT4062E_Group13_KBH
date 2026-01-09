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
            room_manager_.remove_fd(client_fd);
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
        err["message"] = "Failed to join room";
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
    
    room_manager_.remove_fd(fd);
    
    // Room might be deleted if empty
    // If room still exists, broadcast new state
    if (room->player_count() > 0) {
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
    }
}

