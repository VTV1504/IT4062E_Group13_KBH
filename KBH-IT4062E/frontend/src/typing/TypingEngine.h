#pragma once
#include <string>
#include <vector>
#include <jsoncpp/json/json.h>

enum class KeyEventType {
    CHAR,
    BACKSPACE,
    SPACE
};

struct CharEvent {
    int64_t t_ms;           // timestamp ms since game start
    KeyEventType key_type;
    char key_pressed;
};

class TypingEngine {
public:
    explicit TypingEngine(const std::string& paragraph);
    
    // Reset for new game
    void reset();
    
    // Handle key input
    void on_char(char c, int64_t t_ms);
    void on_backspace(int64_t t_ms);
    void on_space(int64_t t_ms);
    
    // Check if word committed (returns true if SPACE committed the word)
    bool is_word_committed() const { return word_committed; }
    
    // Get current word index
    int get_current_word_idx() const { return current_word_idx; }
    
    // Get char events for current word (call after word committed)
    Json::Value get_char_events_json() const;
    
    // Clear events after sending (call after get_char_events_json)
    void clear_events();
    
    // Get current typing state for rendering
    const std::string& get_goal() const { return goal; }
    const std::string& get_curr() const { return curr; }
    bool is_finished() const { return current_word_idx >= (int)words.size(); }
    int get_total_words() const { return words.size(); }
    
    // Get first mismatch position (-1 if all match or curr longer than goal)
    int get_first_mismatch() const;
    
private:
    std::string paragraph;
    std::vector<std::string> words;
    
    int current_word_idx;
    std::string goal;  // target word + ' '
    std::string curr;  // current typed
    
    std::vector<CharEvent> char_events;
    bool word_committed;
    
    void advance_word();
};
