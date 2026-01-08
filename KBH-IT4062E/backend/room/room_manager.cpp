#include "room_manager.h"

#include <random>

RoomManager::RoomManager(Database* db)
    : db_(db) {}

Room* RoomManager::create_room(const std::string& player_name,
                               int fd,
                               std::string& room_id,
                               bool& is_host,
                               std::string& err_msg) {
    if (get_room_of_fd(fd)) {
        err_msg = "ALREADY_IN_ROOM";
        return nullptr;
    }

    room_id = generate_room_code();
    auto room = std::make_unique<Room>(room_id, player_name, fd, db_);
    Room* room_ptr = room.get();
    rooms_[room_id] = std::move(room);
    fd_to_room_[fd] = room_ptr;
    is_host = true;
    err_msg.clear();
    return room_ptr;
}

Room* RoomManager::create_room(const std::string& player_name,
                               int fd,
                               std::string& err_msg,
                               bool& is_host) {
    std::string room_id;
    return create_room(player_name, fd, room_id, is_host, err_msg);
}

Room* RoomManager::join_room_by_code(const std::string& room_id,
                                     const std::string& player_name,
                                     int fd,
                                     bool& is_host,
                                     std::string& err_msg) {
    if (get_room_of_fd(fd)) {
        err_msg = "ALREADY_IN_ROOM";
        return nullptr;
    }

    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) {
        err_msg = "ROOM_NOT_FOUND";
        return nullptr;
    }

    Room* room = it->second.get();
    if (!room->add_player(player_name, fd, err_msg)) {
        return nullptr;
    }

    fd_to_room_[fd] = room;
    is_host = false;
    err_msg.clear();
    return room;
}

Room* RoomManager::join_random_room(const std::string& player_name,
                                    int fd,
                                    bool& is_host,
                                    std::string& err_msg) {
    if (get_room_of_fd(fd)) {
        err_msg = "ALREADY_IN_ROOM";
        return nullptr;
    }

    for (auto& kv : rooms_) {
        Room* room = kv.second.get();
        if (room->is_private() || room->is_full() || room->has_started()) {
            continue;
        }
        if (!room->add_player(player_name, fd, err_msg)) {
            continue;
        }

        fd_to_room_[fd] = room;
        is_host = false;
        err_msg.clear();
        return room;
    }

    err_msg = "NO_PUBLIC_ROOM";
    return nullptr;
}

Room* RoomManager::get_room_of_fd(int fd) const {
    auto it = fd_to_room_.find(fd);
    if (it == fd_to_room_.end()) return nullptr;
    return it->second;
}

RoomManager::RemoveResult RoomManager::remove_fd(int fd) {
    RemoveResult result;
    auto it = fd_to_room_.find(fd);
    if (it == fd_to_room_.end()) return result;

    Room* room = it->second;
    fd_to_room_.erase(it);
    result.room = room;
    result.host_changed = room->remove_player(fd);

    if (room->player_count() == 0) {
        rooms_.erase(room->id());
        result.room_deleted = true;
    }

    return result;
}

Json::Value RoomManager::build_room_state(Room* room) const {
    Json::Value state;
    state["type"] = "room_state";
    state["room_id"] = room->id();
    state["isPrivate"] = room->is_private();
    state["maxPlayers"] = room->max_players();
    state["hostClientId"] = room->host_fd();

    Json::Value slots(Json::arrayValue);
    const auto& players = room->players();
    ArenaMode* arena = room->arena();
    int max_players = room->max_players();
    for (int i = 0; i < max_players; ++i) {
        Json::Value slot;
        slot["slot"] = i;
        if (i < static_cast<int>(players.size())) {
            const auto& player = players[i];
            slot["occupied"] = true;
            slot["clientId"] = player.fd;
            slot["displayName"] = player.name;
            slot["isHost"] = room->is_host(player.fd);
            slot["isReady"] = arena ? arena->is_ready(player.fd) : false;
            slot["knightIndex"] = player.knight_index;
        } else {
            slot["occupied"] = false;
        }
        slots.append(slot);
    }
    state["slot"] = slots;
    state["allReady"] = arena ? arena->all_ready() : false;
    state["canStart"] = (arena && arena->all_ready() && room->player_count() >= 2);
    return state;
}

Json::Value RoomManager::build_game_init(Room* room) const {
    Json::Value init;
    ArenaMode* arena = room->arena();
    init["type"] = "game_init";
    init["paragraph"] = arena ? arena->get_text() : "";
    init["wpm"] = 0.0;
    init["progress"] = build_game_state(room)["progress"];
    init["elapsed"] = 0.0;
    init["ongoing"] = 1;
    return init;
}

Json::Value RoomManager::build_game_state(Room* room) const {
    Json::Value state;
    state["type"] = "game_state";
    state["wpm"] = 0.0;
    state["elapsed"] = 0.0;
    ArenaMode* arena = room->arena();
    Json::Value progress(Json::arrayValue);
    const auto& players = room->players();
    int max_players = room->max_players();
    for (int i = 0; i < max_players; ++i) {
        Json::Value item;
        item["slot"] = i;
        if (i < static_cast<int>(players.size())) {
            const auto& player = players[i];
            item["render"] = 1;
            item["progress"] = arena ? arena->progress_percent(player.fd) : 0;
        } else {
            item["render"] = 0;
            item["progress"] = 0;
        }
        progress.append(item);
    }
    state["progress"] = progress;
    state["ongoing"] = arena ? (arena->finished() ? 0 : 1) : 0;
    return state;
}

std::string RoomManager::generate_room_code() const {
    static constexpr int kLength = 5;
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size() - 1));

    std::string code;
    while (true) {
        code.clear();
        for (int i = 0; i < kLength; ++i) {
            code.push_back(chars[dist(gen)]);
        }
        if (rooms_.find(code) == rooms_.end()) {
            break;
        }
    }

    return code;
}
