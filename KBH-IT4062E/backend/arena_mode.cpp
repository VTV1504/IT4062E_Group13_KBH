#include "arena_mode.h"
#include <algorithm>
#include <sstream>
#include <iostream>

ArenaMode::ArenaMode(const std::string& text) {
    set_target_text(text);
}
ArenaMode::~ArenaMode() {}

void ArenaMode::start() {
    record_start();
}

void ArenaMode::process_input(const std::string& input) {}

bool ArenaMode::add_player(const std::string& player_id) {
    if (players_.size() >= 10) return false; // max 10 players

    ArenaPlayer p;
    p.id = player_id;
    players_.push_back(p);
    return true;
}

void ArenaMode::set_ready(const std::string& player_id) {
    for (auto &p : players_) {
        if (p.id == player_id) {
            p.ready = true;
            break;
        }
    }
}

bool ArenaMode::all_ready() const {
    if (players_.empty()) return false;
    for (const auto& p : players_) {
        if (!p.ready) return false;
    }
    return true;
}

int ArenaMode::ready_count() const {
    int c = 0;
    for (auto& p : players_) if (p.ready) c++;
    return c;
}

void ArenaMode::process_player_result(const std::string& player_id,
                                      const std::string& typed,
                                      double time_seconds)
{
    for (auto &p : players_) {
        if (p.id == player_id) {
            p.result = compute_result_with_duration(typed, time_seconds);
            p.finished = true;
            break;
        }
    }

    if (all_players_finished()) {
        end();
    }
}

bool ArenaMode::all_players_finished() const {
    for (const auto& p : players_) {
        if (!p.finished) return false;
    }
    return true;
}

void ArenaMode::end() {
    std::sort(players_.begin(), players_.end(),
              [](const ArenaPlayer &a, const ArenaPlayer &b) {
                  if (a.result.wpm != b.result.wpm)
                      return a.result.wpm > b.result.wpm;
                  return a.result.accuracy > b.result.accuracy;
              });
}

std::string ArenaMode::get_results_text() const {
    std::ostringstream oss;
    int rank = 1;
    for (const auto &p : players_) {
        oss << rank++ << ". " << p.id
            << " - WPM: " << p.result.wpm
            << " | Acc: " << p.result.accuracy << "%\n";
    }
    return oss.str();
}

void ArenaMode::display_results() {}
