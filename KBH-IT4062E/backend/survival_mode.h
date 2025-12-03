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

    // TODO: Implement start cho chế độ Survival
    void start() override;

    // TODO: Implement xử lý đầu vào cho Survival
    void process_input(const std::string& input) override;

    // TODO: Kết thúc chế độ và lưu kết quả
    void end() override;

    // TODO: Hiển thị kết quả survival
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
