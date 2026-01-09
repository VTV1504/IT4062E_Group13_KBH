#pragma once
#include <string>

enum class GameMode { Training, Arena, Survival };

struct UserInfo {
    std::string username;
};

struct GameResult {
    GameMode mode = GameMode::Training;
    double wpm = 0.0;
    double accuracy = 0.0;
    int score = 0;
};

struct LeaderboardEntry {
    std::string username;
    double value_a = 0.0;
    double value_b = 0.0;
    int points = 0;
    int rooms = 0;
};

struct RankingEntry {
    std::string name;
    bool finished = false;
    size_t progress = 0;
    double finish_time = 0.0;
    double wpm = 0.0;
    double accuracy = 0.0;
    int points = 0;
    int survived_stages = 0;
};
