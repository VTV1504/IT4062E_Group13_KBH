#include "game_manager.h"

GameManager::GameManager() : current_mode(nullptr) {}

GameManager::~GameManager() {
    if (current_mode) {
        delete current_mode;
    }
}

void GameManager::start_game(const std::string& mode) {
    if (mode == "SelfTraining") {
        current_mode = new SelfTrainingMode();
    } else if (mode == "Arena") {
        current_mode = new ArenaMode();
    } else if (mode == "Survival") {
        current_mode = new SurvivalMode();
    } else {
        std::cerr << "Invalid mode\n";
        return;
    }

    current_mode->start();
}

void GameManager::process_input(const std::string& input) {
    if (current_mode) {
        current_mode->process_input(input);
    }
}

void GameManager::end_game() {
    if (current_mode) {
        current_mode->end();
    }
}

void GameManager::display_results() {
    if (current_mode) {
        current_mode->display_results();
    }
}
