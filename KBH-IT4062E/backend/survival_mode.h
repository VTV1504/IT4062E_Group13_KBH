#ifndef SURVIVAL_MODE_H
#define SURVIVAL_MODE_H

#include "game_mode.h"
#include <iostream>
#include <vector>

struct SurvivalPlayer {
    std::string id;
    GameMode::Result last_result;
    bool alive = true;
    int survived_stages = 0;
    int points = 0;
    int eliminated_stage = 0;
};

class SurvivalMode : public GameMode {
public:
    SurvivalMode();
    ~SurvivalMode();

    // Verify >= 5 players and record start time for first stage
    void start() override;

    // Redirect to process_player_result for stage-based handling
    void process_input(const std::string& input) override;

    // Finalize session when top 3 survivors determined, apply placement/bonus points
    void end() override;

    // Print final standings by points and survived stages
    void display_results() override;

    // Player management
    void add_player(const std::string& player_id);

    // Process single player's result for the current stage
    void process_player_result(const std::string& player_id, const std::string& typed, double time_seconds);

    // Advance to next stage (applies stage rules)
    void advance_stage();

    int get_current_stage() const { return current_stage_; }

private:
    int current_stage_ = 1;
    std::vector<SurvivalPlayer> players_;
    // Minimum players to start
    const int min_players_ = 5;
};

#endif
