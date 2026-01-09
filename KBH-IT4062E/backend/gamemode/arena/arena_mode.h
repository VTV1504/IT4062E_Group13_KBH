#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>

#include "game_mode.h"
#include "typing_engine.h"

enum class ArenaDifficulty {
    Easy,
    Medium,
    Hard
};

struct ArenaPlayer {
    int fd;
    std::string name;
    bool ready = false;
    bool finished = false;
    size_t progress = 0;
    double finish_time = 0.0;
    double last_timestamp = 0.0;
};

struct ArenaRankingEntry {
    std::string name;
    bool finished = false;
    size_t progress = 0;
    double finish_time = 0.0;
    double last_timestamp = 0.0;
    double wpm = 0.0;
    double accuracy = 0.0;
};

class ArenaMode : public GameMode {
public:
    ArenaMode();

    void configure(ArenaDifficulty difficulty,
                   const std::string& text,
                   int duration_seconds);

    void start() override;
    void process_input(const std::string& input) override;
    void end() override;
    void display_results() override;

    void add_player(int fd, const std::string& name);
    void remove_player(int fd);

    void set_ready(int fd);
    void set_unready(int fd);
    bool all_ready() const;

    void process_player_key(int fd, char c, double timestamp);
    void finalize_all(double timestamp);

    bool finished() const;
    bool time_over() const;

    std::vector<ArenaRankingEntry> get_rankings() const;
    std::string get_ranking_payload() const;

    const std::string& get_text() const;
    int duration_seconds() const { return duration_seconds_; }

private:
    ArenaDifficulty difficulty_ = ArenaDifficulty::Easy;
    int duration_seconds_ = 90;

    std::unordered_map<int, ArenaPlayer> players_;
    TypingEngine engine_;
    bool started_ = false;
    std::chrono::steady_clock::time_point start_time_;
};
