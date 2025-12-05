#include "game_manager.h"
#include <iostream>

GameManager::GameManager()
    : current_mode(nullptr),
      database(nullptr) {}

GameManager::~GameManager() {
    if (current_mode) {
        delete current_mode;
        current_mode = nullptr;
    }
}

void GameManager::set_database(Database* db) {
    database = db;
}

void GameManager::start_game(const std::string& mode) {
    // xoá mode cũ nếu có
    if (current_mode) {
        delete current_mode;
        current_mode = nullptr;
    }

    if (mode == "Arena") {
        // lấy text từ DB, nếu chưa có DB thì dùng fallback
        std::string text;
        if (database) {
            text = database->get_random_paragraph("en");
        } else {
            text = "fallback text when database is not set.";
        }

        current_mode = new ArenaMode(text);
    }
    else if (mode == "SelfTraining") {
        current_mode = new SelfTrainingMode();
    }
    else if (mode == "Survival") {
        current_mode = new SurvivalMode();
    }
    else {
        std::cerr << "Invalid mode: " << mode << "\n";
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

GameMode* GameManager::get_current_mode() {
    return current_mode;
}
