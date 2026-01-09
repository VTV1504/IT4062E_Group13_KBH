#ifndef SURVIVAL_MODE_H
#define SURVIVAL_MODE_H

#include "game_mode.h"
#include "typing_engine.h"
#include "database.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

struct SurvivalStageRule {
    int word_count;
    int duration_seconds;
    double min_accuracy;
    double min_wpm;
};

struct SurvivalPlayerState {
    int fd;
    std::string name;
    bool alive = true;
    int survived_stages = 0;
    int points = 0;
    int eliminated_stage = 0;
    double last_timestamp = 0.0;
    TypingMetrics last_metrics;
};

class SurvivalMode : public GameMode {
public:
    explicit SurvivalMode(Database* db);

    void start() override;
    void process_input(const std::string& input) override;
    void end() override;
    void display_results() override;

    void add_player(int fd, const std::string& name);
    void remove_player(int fd);

    void set_ready(int fd);
    void set_unready(int fd);
    bool all_ready() const;

    void start_stage();
    void process_player_key(int fd, char c, double timestamp);

    bool stage_finished() const;
    bool time_over() const;

    void evaluate_stage(double timestamp);
    bool tournament_finished() const;

    int current_stage() const { return current_stage_; }
    int duration_seconds() const { return current_rule_.duration_seconds; }
    const std::string& current_text() const { return target_text_; }

    std::vector<SurvivalPlayerState> get_rankings() const;

private:
    SurvivalStageRule rule_for_stage(int stage) const;
    void advance_stage();
    void award_points();

    Database* db_;
    std::unordered_map<int, SurvivalPlayerState> players_;
    TypingEngine engine_;
    SurvivalStageRule current_rule_;
    int current_stage_ = 1;
    bool started_ = false;
    std::chrono::steady_clock::time_point stage_start_;
};

#endif
