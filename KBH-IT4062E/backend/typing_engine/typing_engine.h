#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <jsoncpp/json/json.h>
#include "typing_metrics.h"

// Backend TypingEngine: score from INPUT messages
class TypingEngine {
public:
    explicit TypingEngine(const std::string& paragraph);

    void add_player(int player_id);
    void remove_player(int player_id);

    // Process INPUT message: word_idx + char_events
    void process_input(int player_id, int word_idx, const Json::Value& char_events, int64_t latest_time_ms);

    bool all_finished() const;
    
    struct PlayerMetrics {
        int word_idx = 0;
        int64_t latest_time_ms = 0;
        double wpm = 0.0;
        double accuracy = 0.0;
        double progress = 0.0;
    };
    
    const PlayerMetrics* get_session(int player_id) const;
    int get_total_words() const { return total_words; }

private:
    std::string paragraph;
    std::vector<std::string> words;
    int total_words;
    
    std::unordered_map<int, PlayerMetrics> players;
};
