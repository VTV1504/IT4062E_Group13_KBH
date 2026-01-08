#include "room.h"
#include <algorithm>
#include <iostream>

Room::Room(const std::string& id,
           const std::string& host_name,
           int host_fd,
           Database* db)
    : id_(id),
      host_name_(host_name),
      host_fd_(host_fd),
      arena_(nullptr),
      game_started_(false)
{
    std::string text = "fallback text";
    if (db) {
        text = db->get_random_paragraph("en");
    }

    arena_ = new ArenaMode(text);

    RoomPlayer host{host_name, host_fd, 0};
    players_.push_back(host);

    // ✅ add host to arena (NEW API requires fd + name)
    arena_->add_player(host_fd, host_name);
}

Room::~Room() {
    delete arena_;
}

bool Room::add_player(const std::string& name, int fd, std::string& err_msg) {
    if (is_full()) {
        err_msg = "ROOM_FULL";
        return false;
    }

    for (auto& p : players_) {
        if (p.name == name) {
            err_msg = "NAME_TAKEN";
            return false;
        }
    }

    RoomPlayer rp{name, fd, static_cast<int>(players_.size())};
    players_.push_back(rp);

    // ✅ add player to arena (NEW API: void add_player(int, string))
    arena_->add_player(fd, name);

    err_msg.clear();
    return true;
}

bool Room::remove_player(int fd) {
    bool host_changed = false;
    // ✅ keep arena in sync
    if (arena_) {
        arena_->remove_player(fd);
    }

    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
                       [fd](const RoomPlayer& p) { return p.fd == fd; }),
        players_.end()
    );

    if (host_fd_ == fd && !players_.empty()) {
        host_fd_ = players_.front().fd;
        host_name_ = players_.front().name;
        host_changed = true;
    }

    return host_changed;
}
