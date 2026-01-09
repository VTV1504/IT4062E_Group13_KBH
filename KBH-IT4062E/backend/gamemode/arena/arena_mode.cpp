#include "arena_mode.h"
#include <sstream>
#include <algorithm>

/* ===== constructor ===== */

ArenaMode::ArenaMode(const std::string& text)
    : target_text(text),
      engine(text),
      started(false) {}

/* ===== GameMode overrides ===== */

void ArenaMode::start() {
    started = true;
}

void ArenaMode::process_input(const std::string& /*input*/) {
    // NOT USED
    // ArenaMode nhận input theo player (fd), không theo GameMode interface
}

void ArenaMode::end() {
    // placeholder – logic cleanup nếu cần
}

void ArenaMode::display_results() {
    // server sẽ dùng get_ranking()
}

/* ===== Arena specific logic ===== */

void ArenaMode::add_player(int fd, const std::string& name) {
    players[fd] = ArenaPlayer{fd, name, false};
    engine.add_player(fd);
}

void ArenaMode::remove_player(int fd) {
    players.erase(fd);
    engine.remove_player(fd);
}

void ArenaMode::set_ready(int fd) {
    auto it = players.find(fd);
    if (it != players.end()) {
        it->second.ready = true;
    }
}

void ArenaMode::set_unready(int fd) {
    auto it = players.find(fd);
    if (it != players.end()) {
        it->second.ready = false;
    }
}

bool ArenaMode::all_ready() const {
    if (players.empty()) return false;
    for (const auto& kv : players) {
        if (!kv.second.ready) return false;
    }
    return true;
}

void ArenaMode::process_player_input(int fd, int word_idx, const Json::Value& char_events, int64_t latest_time_ms) {
    if (!started) return;
    engine.process_input(fd, word_idx, char_events, latest_time_ms);
}

bool ArenaMode::finished() const {
    return engine.all_finished();
}

std::string ArenaMode::get_ranking() const {
    struct Result {
        std::string name;
        int word_idx;
        int64_t latest_time_ms;
        double wpm;
        double acc;
    };

    std::vector<Result> results;

    for (const auto& kv : players) {
        const TypingEngine::PlayerMetrics* pm = engine.get_session(kv.first);
        if (!pm) continue;

        results.push_back({
            kv.second.name,
            pm->word_idx,
            pm->latest_time_ms,
            pm->wpm,
            pm->accuracy
        });
    }

    std::sort(results.begin(), results.end(),
              [](const Result& a, const Result& b) {
                  if (a.word_idx != b.word_idx) return a.word_idx > b.word_idx;
                  return a.latest_time_ms < b.latest_time_ms;
              });

    std::ostringstream oss;
    int rank = 1;
    for (const auto& r : results) {
        oss << rank++ << ". "
            << r.name
            << " | Words: " << r.word_idx
            << " | WPM: " << (int)r.wpm
            << " | Acc: " << (int)r.acc << "%\n";
    }

    return oss.str();
}

const std::string& ArenaMode::get_text() const {
    return target_text;
}
