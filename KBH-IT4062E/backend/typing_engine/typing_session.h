#pragma once
#include <string>
#include "typing_metrics.h"

class TypingSession {
public:
    explicit TypingSession(const std::string& target_text);

    // input từng ký tự
    void on_key(char c, double timestamp);

    bool finished() const;
    size_t progress() const;

    const TypingMetrics& metrics() const;

private:
    std::string target;
    size_t cursor;

    double start_time;
    double end_time;

    TypingMetrics stats;
};
