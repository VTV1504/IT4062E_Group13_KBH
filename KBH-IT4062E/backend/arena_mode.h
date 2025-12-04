#ifndef ARENA_MODE_H
#define ARENA_MODE_H

#include "game_mode.h"
#include <iostream>
#include <vector>

struct ArenaPlayer {
    std::string id;
    GameMode::Result result;
    bool finished = false;
};

class ArenaMode : public GameMode {
public:
    explicit ArenaMode(int word_count = 30);
    ~ArenaMode();

    // Initialize arena with target word count and record start time
    void start() override;

    // Redirect to process_player_result for multi-player handling
    void process_input(const std::string& input) override;

    // Rank all players by WPM (desc) and accuracy (tie-breaker)
    void end() override;

    // Print ranked results with WPM and accuracy for each player
    void display_results() override;

    // Add a player to the arena (by id)
    void add_player(const std::string& player_id);

    // Process a single player's result (playerId, typedText, timeSeconds)
    void process_player_result(const std::string& player_id, const std::string& typed, double time_seconds);

private:
    int word_count_ = 30;
    std::vector<ArenaPlayer> players_;
};

#endif
