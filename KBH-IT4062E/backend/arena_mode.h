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

    // TODO: Implement start cho chế độ Arena
    void start() override;

    // TODO: Implement xử lý đầu vào cho Arena
    void process_input(const std::string& input) override;

    // TODO: Kết thúc chế độ và tính toán kết quả
    void end() override;

    // TODO: Hiển thị kết quả đấu trường
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
