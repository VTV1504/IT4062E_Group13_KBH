#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include "arena_mode.h"
#include "database.h"

struct RoomPlayer {
    std::string name;
    int fd;
};

class Room {
public:
    Room(const std::string& id,
         const std::string& host_name,
         int host_fd,
         Database* db);
    ~Room();

    const std::string& id() const { return id_; }
    const std::string& host_name() const { return host_name_; }
    int host_fd() const { return host_fd_; }
    bool is_host(int fd) const { return fd == host_fd_; }

    bool add_player(const std::string& name, int fd, std::string& err_msg);
    void remove_player(int fd);

    int player_count() const { return (int)players_.size(); }
    const std::vector<RoomPlayer>& players() const { return players_; }

    ArenaMode* arena() { return arena_; }

    bool has_started() const { return game_started_; }
    void mark_started() { game_started_ = true; }
    void mark_ended() { game_started_ = false; }

private:
    std::string id_;
    std::string host_name_;
    int host_fd_;
    std::vector<RoomPlayer> players_;
    ArenaMode* arena_;
    bool game_started_ = false;
};

#endif
