#include "game_mode.h"
#include <algorithm>

// Hàm core: dùng start_time_ và end_time_ để tính time_seconds
GameMode::Result GameMode::compute_result(const std::string& typed) const {
    double time_seconds =
        std::chrono::duration_cast<std::chrono::duration<double>>(
            end_time_ - start_time_).count();

    return compute_result_with_duration(typed, time_seconds);
}

// Hàm helper: tính toán khi đã biết time_seconds (ví dụ do client gửi lên)
GameMode::Result GameMode::compute_result_with_duration(
    const std::string& typed,
    double time_seconds
) const {
    Result res;
    res.time_seconds = time_seconds;
    res.total_chars = static_cast<int>(target_text_.size());

    int correct = 0;
    int compare_len = std::min(static_cast<int>(typed.size()), res.total_chars);
    for (int i = 0; i < compare_len; ++i) {
        if (typed[i] == target_text_[i]) ++correct;
    }
    // characters typed beyond target are considered incorrect
    res.correct_chars = correct;

    if (res.total_chars > 0) {
        res.accuracy =
            (static_cast<double>(res.correct_chars) / res.total_chars) * 100.0;
    } else {
        res.accuracy = 100.0;
    }

    // WPM calculation: correct characters / 5 = words typed correctly
    double words = static_cast<double>(res.correct_chars) / 5.0;
    double minutes = (res.time_seconds > 0.0)
                         ? (res.time_seconds / 60.0)
                         : (1.0 / 60.0);
    res.wpm = words / minutes;

    return res;
}
