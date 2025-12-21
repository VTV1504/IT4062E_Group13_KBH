#ifndef GAME_MODE_H
#define GAME_MODE_H

#include <string>
#include <chrono>

class GameMode {
public:
    virtual ~GameMode() {}

    struct Result {
        double wpm = 0.0;
        double accuracy = 0.0; // 0..100
        int correct_chars = 0;
        int total_chars = 0;
        double time_seconds = 0.0;
    };

    // Core lifecycle for a mode
    virtual void start() = 0;
    virtual void process_input(const std::string& input) = 0;
    virtual void end() = 0;
    virtual void display_results() = 0;

protected:
    // Helpers for computing results
    void set_target_text(const std::string& text) { target_text_ = text; }
    void record_start() { start_time_ = std::chrono::steady_clock::now(); }
    void record_end() { end_time_ = std::chrono::steady_clock::now(); }

    // Tính kết quả dựa trên start_time_ / end_time_
    Result compute_result(const std::string& typed) const;

    // Tính kết quả với thời gian (giây) truyền vào trực tiếp
    Result compute_result_with_duration(const std::string& typed,
                                        double time_seconds) const;

    std::string target_text_;
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;
    std::string typed_text_;
};

#endif
