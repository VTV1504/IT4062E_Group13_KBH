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

    // Simple: feed raw key + timestamp (seconds)
    void on_key(int player_id, char c, double timestamp_seconds);

    // Preferred: feed structured batch (for networking + fairness later)
    void on_batch(int player_id, const TypingSession::InputBatch& batch);

    bool all_finished() const;

    const TypingSession* get_session(int player_id) const;

private:
    std::string target_text_;
    std::unordered_map<int, std::unique_ptr<TypingSession>> sessions_;
};
