#pragma once
#include <string>
#include <vector>
#include "typing_metrics.h"

class TypingSession {
public:
    enum class KeyType {
        CHAR,       // normal printable char
        SPACE,      // commit current word (lock it, move next)
        BACKSPACE   // edit within current word only (not counted for accuracy)
    };

    struct InputEvent {
        KeyType type = KeyType::CHAR;
        char ch = '\0';     // only used if type == CHAR
        int dt_ms = 0;      // optional delta time for batch replay
    };

    struct InputBatch {
        double start_ts = 0.0;                 // seconds
        std::vector<InputEvent> events;
    };

public:
    explicit TypingSession(const std::string& target_text);

    // Single key input with timestamp in seconds
    void on_key(char c, double timestamp_seconds);

    // Structured event (recommended for GUI later)
    void on_event(const InputEvent& ev, double timestamp_seconds);

    // Batch replay (recommended for networking later)
    void on_batch(const InputBatch& batch);

    bool finished() const;

    // For UI/debug: which word are we currently typing
    std::size_t current_word_index() const { return word_idx_; }
    std::size_t total_words() const { return words_.size(); }
    const std::string& current_buffer() const { return buffer_; }
    std::string expected_current_word() const;

    // Snapshot metrics (recomputed)
    TypingMetrics metrics() const;

private:
    struct WordStat {
        int correct = 0; // keypress correct
        int total = 0;   // keypress total (excluding backspace)
    };

private:
    void split_words_(const std::string& text);
    void ensure_started_(double ts);
    void handle_char_(char c);
    void handle_backspace_();
    void commit_word_();
    void commit_last_if_needed_(); // when finishing at end (optional)

    double compute_avg_word_accuracy_percent_(bool include_partial_current) const;

private:
    std::vector<std::string> words_;
    std::size_t word_idx_ = 0;

    // Current word typing state
    std::string buffer_;
    std::size_t char_idx_ = 0; // position within expected word

    // Stats
    std::vector<double> committed_word_accuracies_; // per committed word [0..100]
    WordStat current_word_stat_;

    int correct_keypresses_total_ = 0;
    int total_keypresses_total_ = 0;

    // Timing
    double start_time_ = -1.0;
    double last_time_ = -1.0;

    bool finished_ = false;
};
