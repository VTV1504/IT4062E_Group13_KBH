#include "typing_engine.h"
#include <sstream>
#include <algorithm>

TypingEngine::TypingEngine(const std::string& text)
    : paragraph(text), total_words(0) {
    // Normalize and split into words
    std::istringstream iss(paragraph);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    total_words = words.size();
}

void TypingEngine::add_player(int player_id) {
    players[player_id] = PlayerMetrics();
}

void TypingEngine::remove_player(int player_id) {
    players.erase(player_id);
}

void TypingEngine::process_input(int player_id, int word_idx, const Json::Value& char_events, int64_t latest_time_ms) {
    auto it = players.find(player_id);
    if (it == players.end()) return;
    
    if (word_idx < 0 || word_idx >= total_words) return;
    
    PlayerMetrics& pm = it->second;
    
    // Update word_idx and time
    pm.word_idx = std::max(pm.word_idx, word_idx + 1); // +1 because word_idx is committed
    pm.latest_time_ms = latest_time_ms;
    
    // Compute accuracy from char_events
    const std::string& goal = words[word_idx];
    int correct = 0;
    int total = 0;
    
    std::string curr;
    for (const auto& evt : char_events) {
        if (!evt.isMember("key_type") || !evt.isMember("key_pressed")) continue;
        
        std::string key_type = evt["key_type"].asString();
        std::string key_pressed = evt["key_pressed"].asString();
        
        if (key_type == "CHAR") {
            total++;
            curr += key_pressed;
            
            // Check if this character is correct at current position
            size_t pos = curr.size() - 1;
            if (pos < goal.size() && goal[pos] == key_pressed[0]) {
                correct++;
            }
        } else if (key_type == "BACKSPACE") {
            if (!curr.empty()) curr.pop_back();
        }
        // SPACE is not counted
    }
    
    // Update cumulative counters
    pm.total_correct_chars += correct;
    pm.total_chars += total;
    
    // Compute accuracy from cumulative data
    if (pm.total_chars > 0) {
        pm.accuracy = (double)pm.total_correct_chars / pm.total_chars;
    } else {
        pm.accuracy = 1.0;  // 100% if no chars typed yet
    }
    
    // Compute WPM from cumulative correct chars
    if (latest_time_ms > 0) {
        double elapsed_seconds = latest_time_ms / 1000.0;
        if (elapsed_seconds > 0) {
            pm.wpm = (pm.total_correct_chars / 5.0) / (elapsed_seconds / 60.0);
        }
    }
    
    // Progress
    pm.progress = (double)pm.word_idx / total_words;
}

bool TypingEngine::all_finished() const {
    for (const auto& kv : players) {
        if (kv.second.word_idx < total_words) {
            return false;
        }
    }
    return !players.empty();
}

const TypingEngine::PlayerMetrics* TypingEngine::get_session(int player_id) const {
    auto it = players.find(player_id);
    if (it == players.end()) return nullptr;
    return &it->second;
}
