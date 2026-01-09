#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <random>
#include <cctype>

Server::Server(const std::string& ip, int port, Database* db)
    : ip_(ip),
      port_(port),
      server_fd_(-1),
      room_manager_(db),
      db_(db) {
    server_start_ = std::chrono::steady_clock::now();
}

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

    std::cout << "Server running at " << ip_ << ":" << port_ << "\n";

    fd_set read_fds;
    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd_, &read_fds);
        int max_fd = server_fd_;

        for (const auto& entry : clients_) {
            FD_SET(entry.first, &read_fds);
            if (entry.first > max_fd) {
                max_fd = entry.first;
            }
        }

        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(server_fd_, &read_fds)) {
            accept_new_client();
        }

        std::vector<int> disconnects;
        for (auto& entry : clients_) {
            int fd = entry.first;
            if (!FD_ISSET(fd, &read_fds)) {
                continue;
            }

            char buffer[4096];
            int n = recv(fd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) {
                disconnects.push_back(fd);
                continue;
            }

            buffer[n] = '\0';
            entry.second.buffer.append(buffer);

            size_t pos = 0;
            while ((pos = entry.second.buffer.find('\n')) != std::string::npos) {
                std::string line = entry.second.buffer.substr(0, pos);
                entry.second.buffer.erase(0, pos + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (!line.empty()) {
                    handle_message(fd, line);
                }
            }
        }

        for (int fd : disconnects) {
            remove_client(fd);
        }
    }
}

void Server::accept_new_client() {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
        return;
    }

    ClientSession session;
    session.fd = client_fd;
    session.connected_at = std::chrono::steady_clock::now();
    clients_[client_fd] = session;

    Json::Value hello;
    hello["type"] = "CONNECTED";
    hello["message"] = "Welcome to Keyboard Heroes";
    send_json(client_fd, hello);
}

void Server::remove_client(int fd) {
    room_manager_.remove_fd(fd);
    clients_.erase(fd);
    close(fd);
}

void Server::handle_message(int fd, const std::string& msg) {
    std::istringstream iss(msg);
    std::string type;
    iss >> type;
    std::string payload_str;
    std::getline(iss, payload_str);

    if (!payload_str.empty() && payload_str[0] == ' ') {
        payload_str.erase(0, 1);
    }

    Json::Value payload;
    if (!payload_str.empty()) {
        Json::CharReaderBuilder builder;
        std::string errs;
        std::istringstream payload_stream(payload_str);
        if (!Json::parseFromStream(builder, payload_stream, &payload, &errs)) {
            Json::Value error;
            error["type"] = "ERROR";
            error["message"] = "INVALID_JSON";
            send_json(fd, error);
            return;
        }
    }

    handle_command(fd, type, payload);
}

void Server::handle_command(int fd, const std::string& type, const Json::Value& payload) {
    auto client_it = clients_.find(fd);
    if (client_it == clients_.end()) {
        return;
    }
    ClientSession& client = client_it->second;

    if (type == "REGISTER") {
        std::string username = payload.get("username", "").asString();
        std::string password = payload.get("password", "").asString();

        Json::Value response;
        response["type"] = "REGISTER_RESULT";
        if (username.empty() || !validate_password(password)) {
            response["status"] = "FAILED";
            response["message"] = "INVALID_CREDENTIALS";
            send_json(fd, response);
            return;
        }
        if (db_->user_exists(username)) {
            response["status"] = "FAILED";
            response["message"] = "USERNAME_TAKEN";
            send_json(fd, response);
            return;
        }
        std::string salt = generate_salt();
        std::string hashed = hash_password(password, salt);
        if (!db_->create_user(username, hashed, salt)) {
            response["status"] = "FAILED";
            response["message"] = "DB_ERROR";
            send_json(fd, response);
            return;
        }
        response["status"] = "OK";
        send_json(fd, response);
        return;
    }

    if (type == "LOGIN") {
        std::string username = payload.get("username", "").asString();
        std::string password = payload.get("password", "").asString();
        Json::Value response;
        response["type"] = "LOGIN_RESULT";

        std::string salt = db_->get_user_salt(username);
        if (salt.empty()) {
            response["status"] = "FAILED";
            response["message"] = "INVALID_LOGIN";
            send_json(fd, response);
            return;
        }
        std::string hashed = hash_password(password, salt);
        if (!db_->verify_user(username, hashed)) {
            response["status"] = "FAILED";
            response["message"] = "INVALID_LOGIN";
            send_json(fd, response);
            return;
        }

        client.authenticated = true;
        client.is_guest = false;
        client.username = username;

        response["status"] = "OK";
        response["username"] = username;
        send_json(fd, response);
        return;
    }

    if (type == "GUEST") {
        std::string nickname = payload.get("nickname", "Guest").asString();
        client.authenticated = true;
        client.is_guest = true;
        client.username = nickname;

        Json::Value response;
        response["type"] = "GUEST_RESULT";
        response["status"] = "OK";
        response["nickname"] = nickname;
        send_json(fd, response);
        return;
    }

    if (type == "CHANGE_PASSWORD") {
        Json::Value response;
        response["type"] = "CHANGE_PASSWORD_RESULT";
        if (!client.authenticated || client.is_guest) {
            response["status"] = "FAILED";
            response["message"] = "NOT_AUTHENTICATED";
            send_json(fd, response);
            return;
        }
        std::string old_password = payload.get("old_password", "").asString();
        std::string new_password = payload.get("new_password", "").asString();
        if (!validate_password(new_password)) {
            response["status"] = "FAILED";
            response["message"] = "INVALID_PASSWORD";
            send_json(fd, response);
            return;
        }
        std::string salt = db_->get_user_salt(client.username);
        std::string old_hash = hash_password(old_password, salt);
        if (!db_->verify_user(client.username, old_hash)) {
            response["status"] = "FAILED";
            response["message"] = "INVALID_PASSWORD";
            send_json(fd, response);
            return;
        }
        std::string new_salt = generate_salt();
        std::string new_hash = hash_password(new_password, new_salt);
        if (!db_->update_password(client.username, new_hash, new_salt)) {
            response["status"] = "FAILED";
            response["message"] = "DB_ERROR";
            send_json(fd, response);
            return;
        }
        response["status"] = "OK";
        send_json(fd, response);
        return;
    }

    if (!client.authenticated) {
        Json::Value response;
        response["type"] = "ERROR";
        response["message"] = "NOT_AUTHENTICATED";
        send_json(fd, response);
        return;
    }

    if (type == "SELF_TRAINING_START") {
        client.self_training.text = db_->get_paragraph_by_word_count(40);
        client.self_training.session = std::make_unique<TypingSession>(client.self_training.text);
        client.self_training.start_time = std::chrono::steady_clock::now();
        client.self_training.active = true;

        Json::Value response;
        response["type"] = "SELF_TRAINING_READY";
        response["text"] = client.self_training.text;
        response["duration_seconds"] = client.self_training.duration_seconds;
        send_json(fd, response);
        return;
    }

    if (type == "CREATE_ROOM") {
        std::string mode_str = payload.get("mode", "arena").asString();
        std::string visibility_str = payload.get("visibility", "public").asString();

        RoomMode mode = (mode_str == "survival") ? RoomMode::Survival : RoomMode::Arena;
        RoomVisibility visibility = (visibility_str == "private") ? RoomVisibility::Private : RoomVisibility::Public;

        Room* room = room_manager_.create_room(client.username, fd, client.is_guest, mode, visibility);
        Json::Value response;
        response["type"] = "ROOM_CREATED";
        response["room_code"] = room->id();
        response["mode"] = mode_str;
        response["visibility"] = visibility_str;
        response["host"] = client.username;
        send_json(fd, response);
        return;
    }

    if (type == "JOIN_ROOM") {
        std::string room_code = payload.get("room_code", "").asString();
        std::string mode_str = payload.get("mode", "arena").asString();
        bool random_match = payload.get("matchmaking", false).asBool();

        std::string err;
        Room* room = nullptr;
        if (random_match) {
            RoomMode mode = (mode_str == "survival") ? RoomMode::Survival : RoomMode::Arena;
            room = room_manager_.join_random(mode, fd, client.username, client.is_guest, err);
        } else {
            room = room_manager_.join_room(room_code, client.username, fd, client.is_guest, err);
        }

        Json::Value response;
        response["type"] = "ROOM_JOIN_RESULT";
        if (!room) {
            response["status"] = "FAILED";
            response["message"] = err;
            send_json(fd, response);
            return;
        }

        response["status"] = "OK";
        response["room_code"] = room->id();
        response["host"] = room->host_name();
        response["mode"] = (room->mode() == RoomMode::Survival) ? "survival" : "arena";
        send_json(fd, response);
        return;
    }

    if (type == "LEAVE_ROOM") {
        room_manager_.remove_fd(fd);
        Json::Value response;
        response["type"] = "ROOM_LEFT";
        response["status"] = "OK";
        send_json(fd, response);
        return;
    }

    if (type == "READY") {
        Room* room = room_manager_.get_room_of_fd(fd);
        if (!room) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "NOT_IN_ROOM";
            send_json(fd, response);
            return;
        }
        room->set_ready(fd, true);
        Json::Value response;
        response["type"] = "READY_OK";
        send_json(fd, response);

        if (room->all_ready()) {
            Json::Value broadcast_msg;
            broadcast_msg["type"] = "ALL_READY";
            broadcast(room, broadcast_msg);
        }
        return;
    }

    if (type == "UNREADY") {
        Room* room = room_manager_.get_room_of_fd(fd);
        if (!room) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "NOT_IN_ROOM";
            send_json(fd, response);
            return;
        }
        room->set_ready(fd, false);
        Json::Value response;
        response["type"] = "UNREADY_OK";
        send_json(fd, response);
        return;
    }

    if (type == "START_GAME") {
        Room* room = room_manager_.get_room_of_fd(fd);
        if (!room) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "NOT_IN_ROOM";
            send_json(fd, response);
            return;
        }
        if (!room->is_host(fd)) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "ONLY_HOST";
            send_json(fd, response);
            return;
        }
        if (!room->min_players_ready() || !room->all_ready()) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "NOT_READY";
            send_json(fd, response);
            return;
        }

        room->mark_started();
        if (room->mode() == RoomMode::Arena) {
            std::string difficulty = payload.get("difficulty", "easy").asString();
            ArenaDifficulty diff = ArenaDifficulty::Easy;
            int duration = 90;
            int words = 40;
            if (difficulty == "medium") {
                diff = ArenaDifficulty::Medium;
                duration = 120;
                words = 70;
            } else if (difficulty == "hard") {
                diff = ArenaDifficulty::Hard;
                duration = 150;
                words = 100;
            }
            std::string text = db_->get_paragraph_by_word_count(words);
            room->arena()->configure(diff, text, duration);
            room->arena()->start();

            Json::Value start_msg;
            start_msg["type"] = "GAME_START";
            start_msg["mode"] = "arena";
            start_msg["difficulty"] = difficulty;
            start_msg["text"] = text;
            start_msg["duration_seconds"] = duration;
            broadcast(room, start_msg);
        } else {
            room->survival()->start();
            Json::Value start_msg;
            start_msg["type"] = "GAME_START";
            start_msg["mode"] = "survival";
            start_msg["stage"] = room->survival()->current_stage();
            start_msg["text"] = room->survival()->current_text();
            start_msg["duration_seconds"] = room->survival()->duration_seconds();
            broadcast(room, start_msg);
        }
        return;
    }

    if (type == "KEY_INPUT") {
        char key = payload.get("key", "").asString().empty() ? '\0' : payload.get("key", "").asString()[0];
        if (key == '\0') {
            return;
        }

        double timestamp = current_timestamp_seconds();
        Room* room = room_manager_.get_room_of_fd(fd);
        if (room) {
            if (room->mode() == RoomMode::Arena) {
                room->arena()->process_player_key(fd, key, timestamp);
                if (room->arena()->finished() || room->arena()->time_over()) {
                    room->arena()->finalize_all(timestamp);
                    Json::Value result;
                    result["type"] = "GAME_END";
                    Json::Value ranking(Json::arrayValue);
                    for (const auto& entry : room->arena()->get_rankings()) {
                        Json::Value item;
                        item["name"] = entry.name;
                        item["finished"] = entry.finished;
                        item["progress"] = static_cast<Json::UInt64>(entry.progress);
                        item["finish_time"] = entry.finish_time;
                        item["wpm"] = entry.wpm;
                        item["accuracy"] = entry.accuracy;
                        ranking.append(item);
                    }
                    result["ranking"] = ranking;
                    broadcast(room, result);
                    room->mark_ended();
                }
            } else {
                room->survival()->process_player_key(fd, key, timestamp);
                if (room->survival()->stage_finished() || room->survival()->time_over()) {
                    room->survival()->evaluate_stage(timestamp);
                    if (room->survival()->tournament_finished()) {
                        Json::Value result;
                        result["type"] = "GAME_END";
                        Json::Value ranking(Json::arrayValue);
                        auto ranking_data = room->survival()->get_rankings();
                        for (const auto& entry : ranking_data) {
                            Json::Value item;
                            item["name"] = entry.name;
                            item["points"] = entry.points;
                            item["survived_stages"] = entry.survived_stages;
                            ranking.append(item);
                            if (!client.is_guest && client.username == entry.name) {
                                db_->save_survival_result(entry.name, entry.points, entry.survived_stages);
                            }
                        }
                        result["ranking"] = ranking;
                        broadcast(room, result);
                        room->mark_ended();
                    } else {
                        Json::Value stage_msg;
                        stage_msg["type"] = "STAGE_START";
                        stage_msg["stage"] = room->survival()->current_stage();
                        stage_msg["text"] = room->survival()->current_text();
                        stage_msg["duration_seconds"] = room->survival()->duration_seconds();
                        broadcast(room, stage_msg);
                    }
                }
            }
        } else if (client.self_training.active) {
            client.self_training.session->on_key(key, timestamp);
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - client.self_training.start_time).count();
            if (client.self_training.session->finished() || elapsed >= client.self_training.duration_seconds) {
                if (!client.self_training.session->finished()) {
                    client.self_training.session->finalize(timestamp);
                }
                auto metrics = client.self_training.session->metrics();
                Json::Value result;
                result["type"] = "SELF_TRAINING_END";
                result["wpm"] = metrics.wpm();
                result["accuracy"] = metrics.accuracy();
                send_json(fd, result);

                if (!client.is_guest) {
                    db_->save_self_training_result(client.username, metrics.wpm(), metrics.accuracy());
                }
                client.self_training.active = false;
            }
        }
        return;
    }

    if (type == "SELF_TRAINING_LEADERBOARD") {
        if (client.is_guest) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "GUEST_NO_LEADERBOARD";
            send_json(fd, response);
            return;
        }
        Json::Value board = db_->get_self_training_leaderboard(client.username);
        send_json(fd, board);
        return;
    }

    if (type == "SURVIVAL_LEADERBOARD") {
        if (client.is_guest) {
            Json::Value response;
            response["type"] = "ERROR";
            response["message"] = "GUEST_NO_LEADERBOARD";
            send_json(fd, response);
            return;
        }
        Json::Value board = db_->get_survival_leaderboard(client.username);
        send_json(fd, board);
        return;
    }

    Json::Value response;
    response["type"] = "ERROR";
    response["message"] = "UNKNOWN_COMMAND";
    send_json(fd, response);
}

void Server::send_json(int fd, const Json::Value& payload) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string output = Json::writeString(builder, payload);
    output.push_back('\n');
    send(fd, output.c_str(), output.size(), 0);
}

void Server::broadcast(Room* room, const Json::Value& payload) {
    if (!room) return;
    for (const auto& player : room->players()) {
        send_json(player.fd, payload);
    }
}

bool Server::validate_password(const std::string& password) const {
    if (password.size() < 7 || password.size() > 20) {
        return false;
    }
    bool has_upper = false;
    bool has_lower = false;
    bool has_digit = false;
    bool has_special = false;
    for (char c : password) {
        if (std::isupper(static_cast<unsigned char>(c))) has_upper = true;
        else if (std::islower(static_cast<unsigned char>(c))) has_lower = true;
        else if (std::isdigit(static_cast<unsigned char>(c))) has_digit = true;
        else has_special = true;
    }
    return has_upper && has_lower && has_digit && has_special;
}

std::string Server::generate_salt() const {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 61);
    std::string salt;
    for (int i = 0; i < 16; ++i) {
        salt += alphabet[dist(gen)];
    }
    return salt;
}

std::string Server::hash_password(const std::string& password, const std::string& salt) const {
    std::hash<std::string> hasher;
    size_t hash_value = hasher(password + salt);
    return std::to_string(hash_value);
}

double Server::current_timestamp_seconds() const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - server_start_;
    return elapsed.count();
}
