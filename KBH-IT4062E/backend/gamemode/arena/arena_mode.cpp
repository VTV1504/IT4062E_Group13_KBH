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

void ArenaMode::process_player_input(int fd, const std::string& input) {
    if (!started) return;

    // TEMP: feed full string as keystrokes
    double t = 0.0;
    for (char c : input) {
        engine.on_key(fd, c, t);
        t += 0.05;
    }
}

bool ArenaMode::finished() const {
    return engine.all_finished();
}

std::string ArenaMode::get_ranking() const {
    struct Result {
        std::string name;
        double wpm;
        double acc;
    };

    std::vector<Result> results;

    for (const auto& kv : players) {
        const TypingSession* session = engine.get_session(kv.first);
        if (!session) continue;

        const TypingMetrics& m = session->metrics();
        results.push_back({
            kv.second.name,
            m.wpm(),
            m.accuracy()
        });
    }

    std::sort(results.begin(), results.end(),
              [](const Result& a, const Result& b) {
                  return a.wpm > b.wpm;
              });

    std::ostringstream oss;
    int rank = 1;
    for (const auto& r : results) {
        oss << rank++ << ". "
            << r.name
            << " | WPM: " << (int)r.wpm
            << " | Acc: " << (int)r.acc << "%\n";
    }

    return oss.str();
}

const std::string& ArenaMode::get_text() const {
    return target_text;
}
