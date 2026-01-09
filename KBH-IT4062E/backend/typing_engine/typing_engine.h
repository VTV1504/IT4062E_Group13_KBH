#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "typing_session.h"

class TypingEngine {
public:
    explicit TypingEngine(const std::string& text);

    void add_player(int player_id);
    void remove_player(int player_id);

    void on_key(int player_id, char c, double timestamp);

    bool all_finished() const;

    const TypingSession* get_session(int player_id) const;
    TypingSession* get_session_mutable(int player_id);

private:
    std::string target_text;
    std::unordered_map<int, std::unique_ptr<TypingSession>> sessions;
};
