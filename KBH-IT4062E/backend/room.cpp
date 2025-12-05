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

    RoomPlayer host{host_name, host_fd};
    players_.push_back(host);

    // add host to arena
    arena_->add_player(host_name);
}

Room::~Room() {
    delete arena_;
}

bool Room::add_player(const std::string& name, int fd, std::string& err_msg) {
    if (players_.size() >= 10) {
        err_msg = "ROOM_FULL";
        return false;
    }

    for (auto& p : players_) {
        if (p.name == name) {
            err_msg = "NAME_TAKEN";
            return false;
        }
    }

    RoomPlayer rp{name, fd};
    players_.push_back(rp);

    if (!arena_->add_player(name)) {
        err_msg = "ROOM_FULL";
        return false;
    }

    err_msg.clear();
    return true;
}

void Room::remove_player(int fd) {
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
                       [fd](const RoomPlayer& p) { return p.fd == fd; }),
        players_.end()
    );
}
