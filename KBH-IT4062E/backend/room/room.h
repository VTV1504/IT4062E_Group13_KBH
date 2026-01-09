#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <memory>

#include "arena_mode.h"
#include "survival_mode.h"
#include "database.h"

enum class RoomMode {
    Arena,
    Survival
};

enum class RoomVisibility {
    Public,
    Private
};

struct RoomPlayer {
    std::string name;
    int fd;
    bool ready = false;
    int seat = 0;
    bool is_guest = false;
};

class Room {
public:
    Room(const std::string& id,
         RoomMode mode,
         RoomVisibility visibility,
         const std::string& host_name,
         int host_fd,
         Database* db,
         bool host_is_guest);
    ~Room();

    const std::string& id() const { return id_; }
    RoomMode mode() const { return mode_; }
    RoomVisibility visibility() const { return visibility_; }

    const std::string& host_name() const { return host_name_; }
    int host_fd() const { return host_fd_; }
    bool is_host(int fd) const { return fd == host_fd_; }

    bool add_player(const std::string& name, int fd, bool is_guest, std::string& err_msg);
    void remove_player(int fd);

    int player_count() const { return static_cast<int>(players_.size()); }
    const std::vector<RoomPlayer>& players() const { return players_; }

    ArenaMode* arena() { return arena_.get(); }
    SurvivalMode* survival() { return survival_.get(); }

    bool has_started() const { return game_started_; }
    void mark_started() { game_started_ = true; }
    void mark_ended() { game_started_ = false; }

    void set_ready(int fd, bool ready);
    bool all_ready() const;
    bool min_players_ready() const;

    void promote_host_if_needed();

private:
    std::string id_;
    RoomMode mode_;
    RoomVisibility visibility_;
    std::string host_name_;
    int host_fd_;
    std::vector<RoomPlayer> players_;
    std::unique_ptr<ArenaMode> arena_;
    std::unique_ptr<SurvivalMode> survival_;
    bool game_started_ = false;
};

#endif
