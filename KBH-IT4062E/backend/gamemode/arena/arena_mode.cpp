#include "arena_mode.h"
#include <algorithm>
#include <sstream>

ArenaMode::ArenaMode() : engine_("") {}

void ArenaMode::configure(ArenaDifficulty difficulty,
                          const std::string& text,
                          int duration_seconds) {
    difficulty_ = difficulty;
    target_text_ = text;
    duration_seconds_ = duration_seconds;
    engine_ = TypingEngine(target_text_);
}

void ArenaMode::start() {
    started_ = true;
    start_time_ = std::chrono::steady_clock::now();
}

void ArenaMode::process_input(const std::string& /*input*/) {
}

void ArenaMode::end() {
    started_ = false;
}

void ArenaMode::display_results() {
}

void ArenaMode::add_player(int fd, const std::string& name) {
    players_[fd] = ArenaPlayer{fd, name};
    engine_.add_player(fd);
}

void ArenaMode::remove_player(int fd) {
    players_.erase(fd);
    engine_.remove_player(fd);
}

void ArenaMode::set_ready(int fd) {
    auto it = players_.find(fd);
    if (it != players_.end()) {
        it->second.ready = true;
    }
}

void ArenaMode::set_unready(int fd) {
    auto it = players_.find(fd);
    if (it != players_.end()) {
        it->second.ready = false;
    }
}

bool ArenaMode::all_ready() const {
    if (players_.empty()) return false;
    for (const auto& pair : players_) {
        if (!pair.second.ready) return false;
    }
    return true;
}

void ArenaMode::process_player_key(int fd, char c, double timestamp) {
    if (!started_) return;
    auto it = players_.find(fd);
    if (it == players_.end()) return;

    engine_.on_key(fd, c, timestamp);
    const TypingSession* session = engine_.get_session(fd);
    if (!session) return;

    it->second.progress = session->progress();
    it->second.last_timestamp = timestamp;

    if (session->finished() && !it->second.finished) {
        it->second.finished = true;
        it->second.finish_time = timestamp;
    }
}

void ArenaMode::finalize_all(double timestamp) {
    for (auto& pair : players_) {
        TypingSession* session = engine_.get_session_mutable(pair.first);
        if (session && !session->finished()) {
            session->finalize(timestamp);
        }
    }
}

bool ArenaMode::finished() const {
    if (!started_) return false;
    if (players_.empty()) return false;
    for (const auto& pair : players_) {
        if (!pair.second.finished) {
            return false;
        }
    }
    return true;
}

bool ArenaMode::time_over() const {
    if (!started_) return false;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();
    return elapsed >= duration_seconds_;
}

std::vector<ArenaRankingEntry> ArenaMode::get_rankings() const {
    std::vector<ArenaRankingEntry> entries;
    entries.reserve(players_.size());

    for (const auto& pair : players_) {
        const ArenaPlayer& player = pair.second;
        const TypingSession* session = engine_.get_session(player.fd);
        TypingMetrics metrics;
        if (session) {
            metrics = session->metrics();
        }
        ArenaRankingEntry entry;
        entry.name = player.name;
        entry.finished = player.finished;
        entry.progress = player.progress;
        entry.finish_time = player.finish_time;
        entry.last_timestamp = player.last_timestamp;
        entry.wpm = metrics.wpm();
        entry.accuracy = metrics.accuracy();
        entries.push_back(entry);
    }

    std::sort(entries.begin(), entries.end(), [](const ArenaRankingEntry& a, const ArenaRankingEntry& b) {
        if (a.finished != b.finished) {
            return a.finished > b.finished;
        }
        if (a.finished && b.finished) {
            if (a.finish_time != b.finish_time) {
                return a.finish_time < b.finish_time;
            }
            if (a.wpm != b.wpm) {
                return a.wpm > b.wpm;
            }
            return a.accuracy > b.accuracy;
        }
        if (a.progress != b.progress) {
            return a.progress > b.progress;
        }
        if (a.last_timestamp != b.last_timestamp) {
            return a.last_timestamp < b.last_timestamp;
        }
        if (a.wpm != b.wpm) {
            return a.wpm > b.wpm;
        }
        return a.accuracy > b.accuracy;
    });

    return entries;
}

std::string ArenaMode::get_ranking_payload() const {
    auto rankings = get_rankings();
    std::ostringstream oss;
    for (const auto& entry : rankings) {
        oss << entry.name << "|" << (entry.finished ? "finished" : "unfinished")
            << "|" << entry.progress
            << "|" << entry.finish_time
            << "|" << entry.wpm
            << "|" << entry.accuracy << "\n";
    }
    return oss.str();
}

const std::string& ArenaMode::get_text() const {
    return target_text_;
}
