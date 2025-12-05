#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include "room.h"

class RoomManager {
public:
    explicit RoomManager(Database* db);

    // JOIN / tạo room
    // trả về room*, is_host (true nếu là host), err_msg (ROOM_FULL, NAME_TAKEN...)
    Room* join_room(const std::string& room_id,
                    const std::string& player_name,
                    int fd,
                    bool& is_host,
                    std::string& err_msg);

    Room* get_room_of_fd(int fd) const;
    void remove_fd(int fd);

private:
    Database* db_;
    std::unordered_map<std::string, std::unique_ptr<Room>> rooms_;
    std::unordered_map<int, Room*> fd_to_room_;
};

#endif
