#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <jsoncpp/json/json.h>
#include "room.h"

class RoomManager {
public:
    explicit RoomManager(Database* db);

    struct RemoveResult {
        Room* room = nullptr;
        bool room_deleted = false;
        bool host_changed = false;
    };

    // CREATE / JOIN
    Room* create_room(const std::string& player_name,
                      int fd,
                      std::string& room_id,
                      bool& is_host,
                      std::string& err_msg);
    Room* create_room(const std::string& player_name,
                      int fd,
                      std::string& err_msg,
                      bool& is_host);
    Room* join_room_by_code(const std::string& room_id,
                            const std::string& player_name,
                            int fd,
                            bool& is_host,
                            std::string& err_msg);
    Room* join_random_room(const std::string& player_name,
                           int fd,
                           bool& is_host,
                           std::string& err_msg);

    Room* get_room_of_fd(int fd) const;
    RemoveResult remove_fd(int fd);

    Json::Value build_room_state(Room* room) const;
    Json::Value build_game_init(Room* room) const;
    Json::Value build_game_state(Room* room) const;

private:
    std::string generate_room_code() const;

    Database* db_;
    std::unordered_map<std::string, std::unique_ptr<Room>> rooms_;
    std::unordered_map<int, Room*> fd_to_room_;
};

#endif
