#pragma once
#include <cstddef>

struct TypingMetrics {
    // Keypress correctness (excluding BACKSPACE; SPACE is treated as commit, not counted as correctness)
    int correct_keypresses = 0;
    int total_keypresses = 0;

    // Timing
    double elapsed_seconds = 0.0;

    // Progress
    std::size_t words_committed = 0;   // number of fully committed words
    std::size_t total_words = 0;

    // Accuracy definition (Monkeytype-like): average of per-word keypress accuracy
    // Realtime: includes current word as a "partial word" if it has any keypresses.
    double avg_word_accuracy_percent = 100.0;

    // Convenience getters
    double accuracy() const { return avg_word_accuracy_percent; }

    // Standard WPM approximation from correct keypresses:
    // WPM = (correct_chars / 5) / minutes
    // Here we treat "correct_keypresses" as correct characters typed (letters/symbols),
    // excluding backspace and excluding space.
    double wpm() const {
        if (elapsed_seconds <= 0.0) return 0.0;
        const double minutes = elapsed_seconds / 60.0;
        return (correct_keypresses / 5.0) / minutes;
    }
};
