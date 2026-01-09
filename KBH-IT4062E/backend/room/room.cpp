#include "room.h"
#include <algorithm>

Room::Room(const std::string& id,
           RoomMode mode,
           RoomVisibility visibility,
           const std::string& host_name,
           int host_fd,
           Database* db,
           bool host_is_guest)
    : id_(id),
      mode_(mode),
      visibility_(visibility),
      host_name_(host_name),
      host_fd_(host_fd) {
    RoomPlayer host{host_name, host_fd, false, 1, host_is_guest};
    players_.push_back(host);

    if (mode_ == RoomMode::Arena) {
        arena_ = std::make_unique<ArenaMode>();
        arena_->add_player(host_fd_, host_name_);
    } else {
        survival_ = std::make_unique<SurvivalMode>(db);
        survival_->add_player(host_fd_, host_name_);
    }
}

Room::~Room() = default;

bool Room::add_player(const std::string& name, int fd, bool is_guest, std::string& err_msg) {
    if (players_.size() >= 10) {
        err_msg = "ROOM_FULL";
        return false;
    }
    for (const auto& player : players_) {
        if (player.fd == fd) {
            err_msg = "ALREADY_IN_ROOM";
            return false;
        }
    }

    int next_seat = 1;
    for (const auto& player : players_) {
        if (player.seat >= next_seat) {
            next_seat = player.seat + 1;
        }
    }

    RoomPlayer new_player{name, fd, false, next_seat, is_guest};
    players_.push_back(new_player);

    if (arena_) {
        arena_->add_player(fd, name);
    }
    if (survival_) {
        survival_->add_player(fd, name);
    }

    err_msg.clear();
    return true;
}

void Room::remove_player(int fd) {
    auto it = std::remove_if(players_.begin(), players_.end(), [fd](const RoomPlayer& p) {
        return p.fd == fd;
    });
    bool was_host = (fd == host_fd_);
    if (it != players_.end()) {
        players_.erase(it, players_.end());
    }

    if (arena_) {
        arena_->remove_player(fd);
    }
    if (survival_) {
        survival_->remove_player(fd);
    }

    if (was_host) {
        promote_host_if_needed();
    }
}

void Room::set_ready(int fd, bool ready) {
    for (auto& player : players_) {
        if (player.fd == fd) {
            player.ready = ready;
        }
    }
    if (arena_) {
        if (ready) {
            arena_->set_ready(fd);
        } else {
            arena_->set_unready(fd);
        }
    }
    if (survival_) {
        if (ready) {
            survival_->set_ready(fd);
        } else {
            survival_->set_unready(fd);
        }
    }
}

bool Room::all_ready() const {
    if (players_.empty()) return false;
    for (const auto& player : players_) {
        if (!player.ready) {
            return false;
        }
    }
    return true;
}

bool Room::min_players_ready() const {
    if (mode_ == RoomMode::Arena) {
        return players_.size() >= 3;
    }
    return players_.size() >= 6;
}

void Room::promote_host_if_needed() {
    if (players_.empty()) return;
    auto it = std::min_element(players_.begin(), players_.end(), [](const RoomPlayer& a, const RoomPlayer& b) {
        return a.seat < b.seat;
    });
    host_fd_ = it->fd;
    host_name_ = it->name;
}
