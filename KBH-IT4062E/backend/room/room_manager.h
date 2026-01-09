#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include "room.h"

class RoomManager {
public:
    explicit RoomManager(Database* db);

    // Create new room
    Room* create_room(int fd, int client_id, const std::string& display_name);
    
    // Join existing room by ID
    Room* join_room(const std::string& room_id, int fd, int client_id, 
                    const std::string& display_name, std::string& err_msg);
    
    // Join random public room
    Room* join_random(int fd, int client_id, const std::string& display_name);
    
    // Get room by fd
    Room* get_room_of_fd(int fd) const;
    
    // Remove fd from room
    // Returns the Room* if room still has players (for broadcasting),
    // or nullptr if room was deleted (empty)
    Room* remove_fd(int fd);

private:
    std::string generate_room_id();
    
    Database* db_;
    std::unordered_map<std::string, std::unique_ptr<Room>> rooms_;
    std::unordered_map<int, Room*> fd_to_room_;
    int room_counter_ = 1;
};

#endif
