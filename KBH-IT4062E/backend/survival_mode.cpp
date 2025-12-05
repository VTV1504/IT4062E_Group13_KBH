#include "survival_mode.h"
#include <algorithm>

SurvivalMode::SurvivalMode() {}
SurvivalMode::~SurvivalMode() {}

void SurvivalMode::start() {
    std::cout << "Starting Survival Mode...\n";
}

void SurvivalMode::process_input(const std::string& input) {
    std::cout << "SurvivalMode::process_input (not used, use process_player_result)\n";
}

void SurvivalMode::add_player(const std::string& player_id) {
    if (players_.size() >= 10) return;
    SurvivalPlayer p;
    p.id = player_id;
    players_.push_back(p);
}

void SurvivalMode::process_player_result(
    const std::string& player_id, 
    const std::string& typed, 
    double time_seconds
) {
    for (auto &p : players_) {
        if (p.id == player_id && p.alive) {
            p.last_result = compute_result_with_duration(typed, time_seconds);
            p.alive = true;
            break;
        }
    }
}

void SurvivalMode::advance_stage() {
    current_stage_++;
}

void SurvivalMode::end() {
    std::cout << "Ending Survival Mode...\n";
}

void SurvivalMode::display_results() {
    std::cout << "Survival Results:\n";
    for (auto &p : players_) {
        std::cout << p.id << " - WPM: " 
                  << p.last_result.wpm
                  << " | Acc: " << p.last_result.accuracy << "%\n";
    }
}
