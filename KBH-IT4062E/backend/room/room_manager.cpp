#include "room_manager.h"
#include <sstream>
#include <iomanip>

RoomManager::RoomManager(Database* db)
    : db_(db) {}

Room* RoomManager::create_room(int fd, int client_id, const std::string& display_name) {
    // Check if already in a room
    if (get_room_of_fd(fd)) {
        return nullptr;
    }
    
    std::string room_id = generate_room_id();
    auto room = std::make_unique<Room>(room_id, db_);
    Room* room_ptr = room.get();
    
    // Add creator as first player (becomes host)
    if (!room_ptr->add_player(fd, client_id, display_name)) {
        return nullptr;
    }
    
    rooms_[room_id] = std::move(room);
    fd_to_room_[fd] = room_ptr;
    
    return room_ptr;
}

Room* RoomManager::join_room(const std::string& room_id, int fd, int client_id,
                             const std::string& display_name, std::string& err_msg) {
    // Check if already in a room
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
    
    // Try to add player
    if (!room->add_player(fd, client_id, display_name)) {
        err_msg = "ROOM_FULL";
        return nullptr;
    }
    
    fd_to_room_[fd] = room;
    err_msg.clear();
    return room;
}

Room* RoomManager::join_random(int fd, int client_id, const std::string& display_name) {
    // Check if already in a room
    if (get_room_of_fd(fd)) {
        return nullptr;
    }
    
    // Find a public room that is not full
    for (auto& pair : rooms_) {
        Room* room = pair.second.get();
        if (!room->is_private() && room->player_count() < 8) {
            if (room->add_player(fd, client_id, display_name)) {
                fd_to_room_[fd] = room;
                return room;
            }
        }
    }
    
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

    // Delete room if empty
    if (room->player_count() == 0) {
        rooms_.erase(room->id());
    }
}

std::string RoomManager::generate_room_id() {
    // Generate simple alphanumeric ID like "1A2B3C"
    std::ostringstream oss;
    int num = room_counter_++;
    
    // Simple format: uppercase hex
    oss << std::uppercase << std::hex << std::setw(6) << std::setfill('0') << num;
    
    return oss.str();
}
