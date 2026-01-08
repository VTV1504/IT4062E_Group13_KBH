#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <algorithm>
#include <iostream>
#include <sstream>

#include <jsoncpp/json/json.h>

namespace {
Json::Value make_error(const std::string& type, const std::string& msg) {
    Json::Value out;
    out["type"] = "error";
    out["error_type"] = type;
    out["error_msg"] = msg;
    return out;
}

Json::Value make_info(const std::string& type, const std::string& msg) {
    Json::Value out;
    out["type"] = "info";
    out["info_type"] = type;
    out["info_msg"] = msg;
    return out;
}

std::string to_string(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["emitUTF8"] = true;
    return Json::writeString(builder, value) + "\n";
}
}  // namespace

Server::Server(const std::string& ip, int port, Database* db)
    : ip_(ip),
      port_(port),
      server_fd_(-1),
      db_(db),
      room_manager_(db) {}

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

    fd_set master_set;
    FD_ZERO(&master_set);
    FD_SET(server_fd_, &master_set);
    int fd_max = server_fd_;

    while (true) {
        fd_set read_set = master_set;
        int activity = select(fd_max + 1, &read_set, nullptr, nullptr, nullptr);
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        for (int fd = 0; fd <= fd_max; ++fd) {
            if (!FD_ISSET(fd, &read_set)) continue;

            if (fd == server_fd_) {
                int client_fd = accept(server_fd_, nullptr, nullptr);
                if (client_fd < 0) {
                    continue;
                }

                FD_SET(client_fd, &master_set);
                fd_max = std::max(fd_max, client_fd);
                clients_.insert(client_fd);
                recv_buffers_[client_fd];
                client_names_[client_fd] = "Guest_" + std::to_string(client_fd);
            } else {
                char buffer[4096];
                int n = recv(fd, buffer, sizeof(buffer), 0);
                if (n <= 0) {
                    FD_CLR(fd, &master_set);
                    clients_.erase(fd);
                    recv_buffers_.erase(fd);
                    client_names_.erase(fd);
                    handle_disconnect(fd);
                    close(fd);
                    continue;
                }

                recv_buffers_[fd].append(buffer, n);
                auto& pending = recv_buffers_[fd];
                size_t pos = 0;
                while ((pos = pending.find('\n')) != std::string::npos) {
                    std::string line = pending.substr(0, pos);
                    pending.erase(0, pos + 1);
                    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                        line.pop_back();
                    }
                    if (!line.empty()) {
                        handle_message(fd, line);
                    }
                }
            }
        }
    }
}

void Server::broadcast(Room* room, const std::string& msg) {
    if (!room) return;
    for (const auto& p : room->players()) {
        send(p.fd, msg.c_str(), msg.size(), 0);
    }
}

void Server::handle_disconnect(int fd) {
    auto result = room_manager_.remove_fd(fd);
    if (result.room && !result.room_deleted) {
        Json::Value state = room_manager_.build_room_state(result.room);
        broadcast(result.room, to_string(state));
    }
}

void Server::handle_message(int fd, const std::string& msg) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream iss(msg);
    if (!Json::parseFromStream(builder, iss, &root, &errs)) {
        std::string out = to_string(make_error("bad_json", errs));
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    const std::string msg_type = root.get("type", "").asString();
    Json::Value payload = root.isMember("payload") ? root["payload"] : root;

    if (msg_type == "SIGN_IN") {
        const std::string username = payload.get("username", "").asString();
        const std::string passwd = payload.get("passwd", "").asString();
        int user_id = 0;
        if (!db_ || !db_->authenticate(username, passwd, user_id)) {
            std::string out = to_string(make_error("auth_failed", "invalid credentials"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        client_names_[fd] = username;
        Json::Value out;
        out["type"] = "authentication";
        out["client_fd"] = fd;
        out["username"] = username;
        std::string response = to_string(out);
        send(fd, response.c_str(), response.size(), 0);
        return;
    }

    if (msg_type == "CREATE_ACC") {
        const std::string username = payload.get("username", "").asString();
        const std::string passwd = payload.get("passwd", "").asString();
        std::string error;
        if (!db_ || !db_->create_account(username, passwd, error)) {
            std::string out = to_string(make_error("create_account_failed", error.empty() ? "create account failed" : error));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        std::string out = to_string(make_info("create_account_ok", "account created"));
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    if (msg_type == "CHANGE_PASSWD") {
        const std::string username = payload.get("username", client_names_[fd]).asString();
        const std::string old_pwd = payload.get("old_pwd", "").asString();
        const std::string new_pwd = payload.get("new_pwd", "").asString();
        if (!db_ || !db_->change_password(username, old_pwd, new_pwd)) {
            std::string out = to_string(make_error("change_password_failed", "password mismatch"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        std::string out = to_string(make_info("change_password_ok", "password updated"));
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    if (msg_type == "SAVE_RESULT") {
        const std::string username = payload.get("username", client_names_[fd]).asString();
        double wpm = payload.get("wpm", 0.0).asDouble();
        if (!db_ || !db_->save_result(username, wpm)) {
            std::string out = to_string(make_error("save_result_failed", "could not save result"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        std::string out = to_string(make_info("save_result_ok", "result saved"));
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    if (msg_type == "LEADERBOARD") {
        Json::Value response;
        response["type"] = "leaderboard";
        Json::Value top(Json::arrayValue);
        if (db_) {
            auto entries = db_->get_leaderboard_entries();
            for (const auto& entry : entries) {
                Json::Value item;
                item["rank"] = entry.rank;
                item["username"] = entry.username;
                item["wpm"] = entry.wpm;
                top.append(item);
            }
        }
        response["top15"] = top;
        std::string out = to_string(response);
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    if (msg_type == "CREATE") {
        const std::string name = payload.get("username", client_names_[fd]).asString();
        bool is_host = false;
        std::string err;
        std::string room_id;
        Room* room = room_manager_.create_room(name, fd, room_id, is_host, err);
        if (!room) {
            std::string out = to_string(make_error("create_failed", err.empty() ? "create failed" : err));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        Json::Value state = room_manager_.build_room_state(room);
        std::string out = to_string(state);
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    if (msg_type == "JOIN") {
        const std::string join_type = payload.get("join_type", "code").asString();
        const std::string room_id = payload.get("room_id", "").asString();
        const std::string name = payload.get("username", client_names_[fd]).asString();
        bool is_host = false;
        std::string err;
        Room* room = nullptr;

        if (join_type == "random") {
            room = room_manager_.join_random_room(name, fd, is_host, err);
            if (!room) {
                room = room_manager_.create_room(name, fd, err, is_host);
            }
        } else {
            room = room_manager_.join_room_by_code(room_id, name, fd, is_host, err);
        }

        if (!room) {
            std::string out = to_string(make_error("join_failed", err.empty() ? "join failed" : err));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }

        Json::Value state = room_manager_.build_room_state(room);
        std::string out = to_string(state);
        broadcast(room, out);
        return;
    }

    Room* room = room_manager_.get_room_of_fd(fd);
    if (!room) {
        std::string out = to_string(make_error("not_in_room", "client not in room"));
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    ArenaMode* arena = room->arena();
    if (!arena) {
        std::string out = to_string(make_error("no_arena", "arena not available"));
        send(fd, out.c_str(), out.size(), 0);
        return;
    }

    if (msg_type == "EXIT") {
        auto result = room_manager_.remove_fd(fd);
        std::string out = to_string(make_info("room_exited", "left room"));
        send(fd, out.c_str(), out.size(), 0);
        if (result.room && !result.room_deleted) {
            Json::Value state = room_manager_.build_room_state(result.room);
            broadcast(result.room, to_string(state));
        }
        return;
    }

    if (msg_type == "SET_PRIVATE") {
        if (!room->is_host(fd)) {
            std::string out = to_string(make_error("not_host", "only host can set private"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        bool flag = payload.get("private_flag", 0).asBool();
        room->set_private(flag);
        Json::Value state = room_manager_.build_room_state(room);
        broadcast(room, to_string(state));
        return;
    }

    if (msg_type == "READY") {
        arena->set_ready(fd);
        Json::Value state = room_manager_.build_room_state(room);
        broadcast(room, to_string(state));
        return;
    }

    if (msg_type == "UNREADY") {
        arena->set_unready(fd);
        Json::Value state = room_manager_.build_room_state(room);
        broadcast(room, to_string(state));
        return;
    }

    if (msg_type == "START") {
        if (!room->is_host(fd)) {
            std::string out = to_string(make_error("not_host", "only host can start"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        if (room->player_count() < 2) {
            std::string out = to_string(make_error("not_enough_players", "need at least 2 players"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        if (!arena->all_ready()) {
            std::string out = to_string(make_error("not_all_ready", "not all ready"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }
        if (room->has_started()) {
            std::string out = to_string(make_error("already_started", "game already started"));
            send(fd, out.c_str(), out.size(), 0);
            return;
        }

        room->mark_started();
        arena->start();
        Json::Value init = room_manager_.build_game_init(room);
        broadcast(room, to_string(init));
        return;
    }

    if (msg_type == "INPUT") {
        Json::Value event = payload["char_event"];
        std::string key_type = event.get("key_type", "").asString();
        std::string key_press = event.get("key_press", "").asString();
        double timestamp = event.get("timestamp", payload.get("latest_time", 0.0)).asDouble();

        if (key_type == "SPACE") {
            arena->process_player_input(fd, " ", timestamp);
        } else if (key_type == "CHARACTER") {
            if (!key_press.empty()) {
                arena->process_player_input(fd, std::string(1, key_press[0]), timestamp);
            }
        }

        Json::Value state = room_manager_.build_game_state(room);
        broadcast(room, to_string(state));
        if (arena->finished()) {
            room->mark_ended();
        }
        return;
    }

    std::string out = to_string(make_error("unknown_command", "unknown command"));
    send(fd, out.c_str(), out.size(), 0);
}
