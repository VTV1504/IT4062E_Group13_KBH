#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <string>
#include "game_mode.h"
#include "self_training_mode.h"
#include "arena_mode.h"
#include "survival_mode.h"
#include "database.h"

class GameManager {
public:
    GameManager();
    ~GameManager();

    // gán con trỏ Database từ bên ngoài (main)
    void set_database(Database* db);

    void start_game(const std::string& mode);
    void process_input(const std::string& input);
    void end_game();
    void display_results();

    GameMode* get_current_mode();

private:
    GameMode* current_mode;
    Database* database;   // có thể nullptr nếu chưa set
};

#endif
