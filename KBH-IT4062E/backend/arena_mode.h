#ifndef ARENA_MODE_H
#define ARENA_MODE_H

#include "game_mode.h"
#include <string>
#include <vector>

struct ArenaPlayer {
    std::string id;
    bool ready = false;
    bool finished = false;
    GameMode::Result result;
};

class ArenaMode : public GameMode {
public:
    explicit ArenaMode(const std::string& text);
    ~ArenaMode();

    void start() override;
    void process_input(const std::string& input) override;
    void end() override;
    void display_results() override;

    bool add_player(const std::string& player_id);
    void set_ready(const std::string& player_id);

    bool all_ready() const;
    int ready_count() const;
    int get_player_count() const { return players_.size(); }

    void process_player_result(const std::string& player_id,
                               const std::string& typed,
                               double time_seconds);

    bool all_players_finished() const;
    std::string get_results_text() const;

    std::string get_target_text() const { return target_text_; }

private:
    std::vector<ArenaPlayer> players_;
};

#endif
