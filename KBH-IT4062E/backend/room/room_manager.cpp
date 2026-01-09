#include "room_manager.h"
#include <random>

RoomManager::RoomManager(Database* db)
    : db_(db) {}

Room* RoomManager::create_room(const std::string& host_name,
                               int host_fd,
                               bool host_is_guest,
                               RoomMode mode,
                               RoomVisibility visibility) {
    std::string room_id = generate_room_code();
    auto room = std::make_unique<Room>(room_id, mode, visibility, host_name, host_fd, db_, host_is_guest);
    Room* room_ptr = room.get();
    rooms_[room_id] = std::move(room);
    fd_to_room_[host_fd] = room_ptr;
    return room_ptr;
}

Room* RoomManager::join_room(const std::string& room_id,
                             const std::string& player_name,
                             int fd,
                             bool is_guest,
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
    if (!room->add_player(player_name, fd, is_guest, err_msg)) {
        return nullptr;
    }

    fd_to_room_[fd] = room;
    err_msg.clear();
    return room;
}

Room* RoomManager::join_random(RoomMode mode, int fd, const std::string& name, bool is_guest, std::string& err_msg) {
    for (auto& pair : rooms_) {
        Room* room = pair.second.get();
        if (room->mode() != mode) continue;
        if (room->visibility() != RoomVisibility::Public) continue;
        if (room->has_started()) continue;
        if (room->player_count() >= 10) continue;

        if (room->add_player(name, fd, is_guest, err_msg)) {
            fd_to_room_[fd] = room;
            return room;
        }
    }
    err_msg = "NO_ROOM_AVAILABLE";
    return nullptr;
}

Room* RoomManager::get_room_of_fd(int fd) const {
    auto it = fd_to_room_.find(fd);
    if (it == fd_to_room_.end()) return nullptr;
    return it->second;
}

void RoomManager::remove_fd(int fd) {
    auto it = fd_to_room_.find(fd);
    if (it == fd_to_room_.end()) return;

    Room* room = it->second;
    fd_to_room_.erase(it);

    room->remove_player(fd);

    if (room->player_count() == 0) {
        rooms_.erase(room->id());
    }
}

Room* RoomManager::find_room(const std::string& room_id) const {
    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) return nullptr;
    return it->second.get();
}

std::string RoomManager::generate_room_code() const {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 35);

    std::string code;
    do {
        code.clear();
        for (int i = 0; i < 6; ++i) {
            code += alphabet[dist(gen)];
        }
    } while (rooms_.find(code) != rooms_.end());

    return code;
}
