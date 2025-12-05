#ifndef ARENA_MODE_H
#define ARENA_MODE_H

#include "game_mode.h"
#include <string>
#include <vector>

struct ArenaPlayer {
    std::string id;
    GameMode::Result result;
    bool finished = false;
};

class ArenaMode : public GameMode {
public:
    explicit ArenaMode(const std::string& text);
    ~ArenaMode();

    void start() override;
    void process_input(const std::string& input) override;
    void end() override;
    void display_results() override;

    void add_player(const std::string& player_id);
    void process_player_result(const std::string& player_id,
                               const std::string& typed,
                               double time_seconds);

    bool all_players_finished() const;
    std::string get_results_text() const;

    int get_player_count() const { return players_.size(); }
    std::string get_target_text() const { return target_text_; }

private:
    std::vector<ArenaPlayer> players_;
};

#endif
