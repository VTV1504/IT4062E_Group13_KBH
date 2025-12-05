#include "arena_mode.h"
#include <algorithm>
#include <sstream>
#include <iostream>

ArenaMode::ArenaMode(const std::string& text) {
    set_target_text(text);
}

ArenaMode::~ArenaMode() {}

void ArenaMode::start() {
    std::cout << "[Arena] Starting with " << players_.size() << " players\n";
    record_start();
}

void ArenaMode::process_input(const std::string& input) {}

void ArenaMode::add_player(const std::string& player_id) {
    ArenaPlayer p;
    p.id = player_id;
    players_.push_back(p);
}

void ArenaMode::process_player_result(const std::string& player_id,
                                      const std::string& typed,
                                      double time_seconds)
{
    for (auto& p : players_) {
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
    if (players_.empty()) return false;
    for (const auto& p : players_) {
        if (!p.finished) return false;
    }
    return true;
}

void ArenaMode::end() {
    std::sort(players_.begin(), players_.end(),
              [](const ArenaPlayer& a, const ArenaPlayer& b) {
                  if (a.result.wpm != b.result.wpm)
                      return a.result.wpm > b.result.wpm;
                  return a.result.accuracy > b.result.accuracy;
              });
}

std::string ArenaMode::get_results_text() const {
    std::ostringstream oss;
    int rank = 1;
    for (const auto& p : players_) {
        oss << rank++ << ". " << p.id
            << " - WPM: " << p.result.wpm
            << " | Acc: " << p.result.accuracy << "%\n";
    }
    return oss.str();
}

void ArenaMode::display_results() {}
