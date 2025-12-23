#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "game_mode.h"
#include "typing_engine.h"

struct ArenaPlayer {
    int fd;
    std::string name;
    bool ready = false;
};

class ArenaMode : public GameMode {
public:
    explicit ArenaMode(const std::string& text);

    /* ===== lifecycle from GameMode ===== */
    void start() override;
    void process_input(const std::string& input) override;   // unused, required by interface
    void end() override;
    void display_results() override;

    /* ===== Arena specific APIs (used by Server / Room) ===== */
    void add_player(int fd, const std::string& name);
    void remove_player(int fd);

    void set_ready(int fd);
    bool all_ready() const;

    void process_player_input(int fd, const std::string& input);

    bool finished() const;
    std::string get_ranking() const;

    const std::string& get_text() const;

private:
    std::string target_text;
    std::unordered_map<int, ArenaPlayer> players;

    TypingEngine engine;
    bool started = false;
};
