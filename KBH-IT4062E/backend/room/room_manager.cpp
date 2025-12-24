#include "room_manager.h"

RoomManager::RoomManager(Database* db)
    : db_(db) {}

Room* RoomManager::join_room(const std::string& room_id,
                             const std::string& player_name,
                             int fd,
                             bool& is_host,
                             std::string& err_msg)
{
    if(get_room_of_fd(fd)) {
        err_msg = "ALREADY_IN_ROOM";
        return nullptr;
    }
    auto it = rooms_.find(room_id);
    
    if (it == rooms_.end()) {
        // Tạo room mới, player này là host
        auto room = std::make_unique<Room>(room_id, player_name, fd, db_);
        Room* room_ptr = room.get();
        rooms_[room_id] = std::move(room);
        fd_to_room_[fd] = room_ptr;
        is_host = true;
        err_msg.clear();
        return room_ptr;
    } else {
        Room* room = it->second.get();
        is_host = false;

        if (!room->add_player(player_name, fd, err_msg)) {
            return nullptr;
        }

        fd_to_room_[fd] = room;
        err_msg.clear();
        return room;
    }
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
        // xoá room nếu không còn ai
        rooms_.erase(room->id());
    }
}
