#include "typing_session.h"
#include <cctype>
#include <sstream>
#include <algorithm>

TypingSession::TypingSession(const std::string& target_text) {
    split_words_(target_text);
}

void TypingSession::split_words_(const std::string& text) {
    words_.clear();

    // Split by whitespace, collapse multiple spaces/newlines/tabs
    std::istringstream iss(text);
    std::string w;
    while (iss >> w) {
        words_.push_back(w);
    }

    // Reset state
    word_idx_ = 0;
    buffer_.clear();
    char_idx_ = 0;
    committed_word_accuracies_.clear();
    current_word_stat_ = {};
    correct_keypresses_total_ = 0;
    total_keypresses_total_ = 0;
    start_time_ = -1.0;
    last_time_ = -1.0;
    finished_ = (words_.empty());
}

void TypingSession::ensure_started_(double ts) {
    if (start_time_ < 0.0) start_time_ = ts;
    last_time_ = ts;
}

std::string TypingSession::expected_current_word() const {
    if (words_.empty() || word_idx_ >= words_.size()) return std::string();
    return words_[word_idx_];
}

bool TypingSession::finished() const {
    return finished_;
}

void TypingSession::on_key(char c, double timestamp_seconds) {
    if (finished_) return;
    ensure_started_(timestamp_seconds);

    // Normalize Enter to SPACE for terminal tests (optional)
    if (c == '\n' || c == '\r') {
        on_event({KeyType::SPACE, '\0', 0}, timestamp_seconds);
        return;
    }

    // Common backspace codes in terminals
    if (c == 127 || c == 8) {
        on_event({KeyType::BACKSPACE, '\0', 0}, timestamp_seconds);
        return;
    }

    if (c == ' ') {
        on_event({KeyType::SPACE, '\0', 0}, timestamp_seconds);
        return;
    }

    on_event({KeyType::CHAR, c, 0}, timestamp_seconds);
}

void TypingSession::on_event(const InputEvent& ev, double timestamp_seconds) {
    if (finished_) return;
    ensure_started_(timestamp_seconds);

    switch (ev.type) {
        case KeyType::BACKSPACE:
            handle_backspace_();
            break;
        case KeyType::SPACE:
            commit_word_();
            break;
        case KeyType::CHAR:
        default:
            handle_char_(ev.ch);
            break;
    }
}

void TypingSession::on_batch(const InputBatch& batch) {
    if (finished_) return;

    // If first batch, start at batch.start_ts
    double t = batch.start_ts;
    ensure_started_(t);

    for (const auto& ev : batch.events) {
        t = batch.start_ts + (ev.dt_ms / 1000.0);
        on_event(ev, t);
        if (finished_) break;
    }
}

void TypingSession::handle_char_(char c) {
    if (words_.empty() || word_idx_ >= words_.size()) {
        finished_ = true;
        return;
    }

    const std::string& expected = words_[word_idx_];

    // We record correctness at time of keypress.
    // Backspace does NOT erase past mistakes from accuracy history.
    // Only CHAR inputs count towards accuracy.
    current_word_stat_.total++;
    total_keypresses_total_++;

    bool correct = false;
    if (char_idx_ < expected.size() && c == expected[char_idx_]) {
        correct = true;
    }
    if (correct) {
        current_word_stat_.correct++;
        correct_keypresses_total_++;
    }

    // Update editing buffer (for UI / current correctness display)
    buffer_.push_back(c);
    char_idx_++;

    // Note: We do NOT auto-commit when reaching expected length.
    // Monkeytype-style expects SPACE to commit the word.
    // (You can optionally auto-commit if you want later.)
}

void TypingSession::handle_backspace_() {
    // Can only backspace within current word buffer (not across committed words)
    if (buffer_.empty()) return;

    buffer_.pop_back();
    if (char_idx_ > 0) char_idx_--;

    // Important: Backspace should NOT change accuracy totals (per your requirement).
    // So we do NOT decrement any correct/total counters here.
}

double TypingSession::compute_avg_word_accuracy_percent_(bool include_partial_current) const {
    // Average per word. For realtime, include current word as partial if it has any keypresses.
    std::vector<double> accs = committed_word_accuracies_;

    if (include_partial_current) {
        if (current_word_stat_.total > 0) {
            double cur = 100.0 * (double)current_word_stat_.correct / (double)current_word_stat_.total;
            accs.push_back(cur);
        }
    }

    if (accs.empty()) return 100.0;

    double sum = 0.0;
    for (double a : accs) sum += a;
    return sum / (double)accs.size();
}

void TypingSession::commit_word_() {
    if (words_.empty() || word_idx_ >= words_.size()) {
        finished_ = true;
        return;
    }

    // Compute per-word accuracy for this word based on keypress history for the word
    double word_acc = 100.0;
    if (current_word_stat_.total > 0) {
        word_acc = 100.0 * (double)current_word_stat_.correct / (double)current_word_stat_.total;
    }

    committed_word_accuracies_.push_back(word_acc);

    // Move to next word; lock previous word (no backspace across words)
    word_idx_++;
    buffer_.clear();
    char_idx_ = 0;
    current_word_stat_ = {};

    if (word_idx_ >= words_.size()) {
        finished_ = true;
    }
}

TypingMetrics TypingSession::metrics() const {
    TypingMetrics m;
    m.correct_keypresses = correct_keypresses_total_;
    m.total_keypresses = total_keypresses_total_;
    m.words_committed = committed_word_accuracies_.size();
    m.total_words = words_.size();

    if (start_time_ >= 0.0 && last_time_ >= 0.0 && last_time_ >= start_time_) {
        m.elapsed_seconds = last_time_ - start_time_;
    } else {
        m.elapsed_seconds = 0.0;
    }

    // Realtime accuracy: average per-word, include current partial word if any keypresses
    m.avg_word_accuracy_percent = compute_avg_word_accuracy_percent_(true);

    return m;
}
