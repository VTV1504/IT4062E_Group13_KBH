#include "arena_mode.h"

#include <algorithm>

ArenaMode::ArenaMode(int word_count) : word_count_(word_count) {
    // generate a simple placeholder target text: repeated "word"
    std::string text;
    for (int i = 0; i < word_count_; ++i) {
        if (i) text += ' ';
        text += "word";
    }
    set_target_text(text);
}

ArenaMode::~ArenaMode() {}

void ArenaMode::start() {
    std::cout << "Starting Arena Mode with " << word_count_ << " words...\n";
    record_start();
}

void ArenaMode::process_input(const std::string& input) {
    // Generic processing not used for multi-player; prefer process_player_result
    std::cout << "ArenaMode::process_input called (use process_player_result instead)\n";
}

void ArenaMode::add_player(const std::string& player_id) {
    if (players_.size() >= 10) return;
    ArenaPlayer p;
    p.id = player_id;
    players_.push_back(p);
}

void ArenaMode::process_player_result(const std::string& player_id, const std::string& typed, double time_seconds) {
    // find player
    for (auto &p : players_) {
        if (p.id == player_id) {
            // record per-player timing: we simulate times by using start_time_ + time_seconds
            // Temporarily set local start/end times to compute result
            auto now = std::chrono::steady_clock::now();
            // set a fake duration by setting end_time_ = now and start_time_ = now - time_seconds
            // NOTE: compute_result uses start_time_ and end_time_ fields in GameMode, so we set them here
            // Save originals
            auto orig_start = start_time_;
            auto orig_end = end_time_;
            end_time_ = now;
            start_time_ = now - std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(time_seconds));
            p.result = compute_result(typed);
            p.finished = true;
            // restore
            start_time_ = orig_start;
            end_time_ = orig_end;
            break;
        }
    }

    // if all finished => end
    bool all = !players_.empty() && std::all_of(players_.begin(), players_.end(), [](const ArenaPlayer& ap){ return ap.finished; });
    if (all) end();
}

void ArenaMode::end() {
    std::cout << "Ending Arena Mode...\n";
    // compute placements by WPM desc, tie-breaker accuracy
    std::sort(players_.begin(), players_.end(), [](const ArenaPlayer &a, const ArenaPlayer &b){
        if (a.result.wpm != b.result.wpm) return a.result.wpm > b.result.wpm;
        return a.result.accuracy > b.result.accuracy;
    });
}

void ArenaMode::display_results() {
    std::cout << "Arena Results:\n";
    int rank = 1;
    for (const auto &p : players_) {
        std::cout << rank++ << ". " << p.id << " - WPM: " << p.result.wpm << " | Acc: " << p.result.accuracy << "%\n";
    }
}

