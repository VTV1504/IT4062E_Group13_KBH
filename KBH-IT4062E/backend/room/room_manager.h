#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include "room.h"

class RoomManager {
public:
    explicit RoomManager(Database* db);

    Room* create_room(const std::string& host_name,
                      int host_fd,
                      bool host_is_guest,
                      RoomMode mode,
                      RoomVisibility visibility);

    Room* join_room(const std::string& room_id,
                    const std::string& player_name,
                    int fd,
                    bool is_guest,
                    std::string& err_msg);

    Room* join_random(RoomMode mode, int fd, const std::string& name, bool is_guest, std::string& err_msg);

    Room* get_room_of_fd(int fd) const;
    void remove_fd(int fd);

    Room* find_room(const std::string& room_id) const;

private:
    std::string generate_room_code() const;

    Database* db_;
    std::unordered_map<std::string, std::unique_ptr<Room>> rooms_;
    std::unordered_map<int, Room*> fd_to_room_;
};

#endif
