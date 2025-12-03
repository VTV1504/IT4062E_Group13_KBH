#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "game_mode.h"
#include "self_training_mode.h"
#include "arena_mode.h"
#include "survival_mode.h"

class GameManager {
public:
    GameManager();
    ~GameManager();

    // TODO: Hàm để khởi tạo chế độ chơi
    void start_game(const std::string& mode);
    void process_input(const std::string& input);
    void end_game();
    void display_results();

private:
    GameMode* current_mode;  // Con trỏ GameMode để linh hoạt chọn chế độ chơi
};

#endif
