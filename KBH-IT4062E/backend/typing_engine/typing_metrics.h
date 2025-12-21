#pragma once

struct TypingMetrics {
    int correct = 0;
    int incorrect = 0;
    double elapsed_seconds = 0.0;

    double accuracy() const {
        int total = correct + incorrect;
        if (total == 0) return 100.0;
        return (double)correct * 100.0 / total;
    }

    double wpm() const {
        if (elapsed_seconds <= 0.0) return 0.0;
        return (correct / 5.0) / (elapsed_seconds / 60.0);
    }
};
