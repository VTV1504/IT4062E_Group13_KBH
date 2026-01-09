#include "TypingEngine.h"
#include <sstream>
#include <algorithm>

TypingEngine::TypingEngine(const std::string& paragraph)
    : paragraph(paragraph), current_word_idx(0), word_committed(false) {
    
    // Normalize and split into words
    std::istringstream iss(paragraph);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    
    if (!words.empty()) {
        goal = words[0] + ' ';
    }
}

void TypingEngine::reset() {
    current_word_idx = 0;
    curr.clear();
    char_events.clear();
    word_committed = false;
    
    if (!words.empty()) {
        goal = words[0] + ' ';
    }
}

void TypingEngine::on_char(char c, int64_t t_ms) {
    if (is_finished()) return;
    
    word_committed = false;
    curr += c;
    char_events.push_back({t_ms, KeyEventType::CHAR, c});
}

void TypingEngine::on_backspace(int64_t t_ms) {
    if (is_finished() || curr.empty()) return;
    
    word_committed = false;
    curr.pop_back();
    char_events.push_back({t_ms, KeyEventType::BACKSPACE, '\b'});
}

void TypingEngine::on_space(int64_t t_ms) {
    if (is_finished()) return;
    
    word_committed = false;
    
    // Push space into curr
    curr += ' ';
    char_events.push_back({t_ms, KeyEventType::SPACE, ' '});
    
    // Check if goal == curr
    if (goal == curr) {
        // Commit word
        word_committed = true;
        current_word_idx++;
        advance_word();
    }
    // If not equal: do not advance (prevents skipping)
}

void TypingEngine::advance_word() {
    curr.clear();
    
    if (current_word_idx < (int)words.size()) {
        goal = words[current_word_idx] + ' ';
    } else {
        goal.clear();
    }
}

Json::Value TypingEngine::get_char_events_json() const {
    Json::Value arr(Json::arrayValue);
    
    for (const auto& evt : char_events) {
        Json::Value obj;
        obj["t_ms"] = (Json::Int64)evt.t_ms;
        
        switch (evt.key_type) {
            case KeyEventType::CHAR:
                obj["key_type"] = "CHAR";
                break;
            case KeyEventType::BACKSPACE:
                obj["key_type"] = "BACKSPACE";
                break;
            case KeyEventType::SPACE:
                obj["key_type"] = "SPACE";
                break;
        }
        
        obj["key_pressed"] = std::string(1, evt.key_pressed);
        arr.append(obj);
    }
    
    return arr;
}

void TypingEngine::clear_events() {
    char_events.clear();
}

int TypingEngine::get_first_mismatch() const {
    size_t min_len = std::min(goal.size(), curr.size());
    
    for (size_t i = 0; i < min_len; ++i) {
        if (goal[i] != curr[i]) {
            return (int)i;
        }
    }
    
    // If curr is longer than goal, first mismatch is at goal.size()
    if (curr.size() > goal.size()) {
        return (int)goal.size();
    }
    
    // All match
    return -1;
}
