#pragma once
#include <string>

enum class GameMode { Training, Arena };

struct UserInfo {
    std::string username;
};

struct GameResult {
    GameMode mode = GameMode::Training;
    int wpm = 0;
    float accuracy = 0.0f;
    int score = 0;
};
