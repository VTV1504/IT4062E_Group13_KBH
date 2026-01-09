#include "typing_session.h"

TypingSession::TypingSession(const std::string& target_text)
    : target(target_text),
      cursor(0),
      start_time(-1.0),
      end_time(-1.0) {}

void TypingSession::on_key(char c, double timestamp) {
    if (finished()) return;

    if (start_time < 0) {
        start_time = timestamp;
    }

    if (cursor < target.size()) {
        if (c == target[cursor]) {
            stats.correct++;
        } else {
            stats.incorrect++;
        }
        cursor++;
    }

    if (cursor >= target.size()) {
        end_time = timestamp;
        stats.elapsed_seconds = end_time - start_time;
    }
}

void TypingSession::finalize(double timestamp) {
    if (start_time < 0) {
        start_time = timestamp;
    }
    end_time = timestamp;
    stats.elapsed_seconds = end_time - start_time;
}

bool TypingSession::finished() const {
    return cursor >= target.size();
}

size_t TypingSession::progress() const {
    return cursor;
}

const TypingMetrics& TypingSession::metrics() const {
    return stats;
}
