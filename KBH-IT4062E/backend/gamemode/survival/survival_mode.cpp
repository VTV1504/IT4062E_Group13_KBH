#include "survival_mode.h"
#include <algorithm>

SurvivalMode::SurvivalMode(Database* db)
    : db_(db), engine_("") {
    current_rule_ = rule_for_stage(current_stage_);
}

void SurvivalMode::start() {
    started_ = true;
    start_stage();
}

void SurvivalMode::process_input(const std::string& /*input*/) {
}

void SurvivalMode::end() {
    started_ = false;
}

void SurvivalMode::display_results() {
}

void SurvivalMode::add_player(int fd, const std::string& name) {
    players_[fd] = SurvivalPlayerState{fd, name};
}

void SurvivalMode::remove_player(int fd) {
    players_.erase(fd);
    engine_.remove_player(fd);
}

void SurvivalMode::set_ready(int fd) {
    auto it = players_.find(fd);
    if (it != players_.end()) {
        it->second.last_metrics = TypingMetrics{};
    }
}

void SurvivalMode::set_unready(int fd) {
    auto it = players_.find(fd);
    if (it != players_.end()) {
        it->second.last_metrics = TypingMetrics{};
    }
}

bool SurvivalMode::all_ready() const {
    if (players_.size() < 6) {
        return false;
    }
    return true;
}

void SurvivalMode::start_stage() {
    current_rule_ = rule_for_stage(current_stage_);
    if (db_) {
        target_text_ = db_->get_paragraph_by_word_count(current_rule_.word_count);
    }
    engine_ = TypingEngine(target_text_);
    for (const auto& pair : players_) {
        if (pair.second.alive) {
            engine_.add_player(pair.first);
        }
    }
    stage_start_ = std::chrono::steady_clock::now();
}

void SurvivalMode::process_player_key(int fd, char c, double timestamp) {
    if (!started_) return;
    auto it = players_.find(fd);
    if (it == players_.end() || !it->second.alive) return;

    engine_.on_key(fd, c, timestamp);
    const TypingSession* session = engine_.get_session(fd);
    if (!session) return;

    it->second.last_metrics = session->metrics();
    it->second.last_timestamp = timestamp;
}

bool SurvivalMode::stage_finished() const {
    if (!started_) return false;
    for (const auto& pair : players_) {
        if (!pair.second.alive) continue;
        const TypingSession* session = engine_.get_session(pair.first);
        if (!session || !session->finished()) {
            return false;
        }
    }
    return true;
}

bool SurvivalMode::time_over() const {
    if (!started_) return false;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - stage_start_).count();
    return elapsed >= current_rule_.duration_seconds;
}

void SurvivalMode::evaluate_stage(double timestamp) {
    for (auto& pair : players_) {
        auto& player = pair.second;
        if (!player.alive) continue;

        TypingSession* session = engine_.get_session_mutable(pair.first);
        bool finished = session && session->finished();
        if (session && !finished) {
            session->finalize(timestamp);
        }
        if (session) {
            player.last_metrics = session->metrics();
        }
        double accuracy = player.last_metrics.accuracy();
        double wpm = player.last_metrics.wpm();

        if (!finished || accuracy < current_rule_.min_accuracy || wpm < current_rule_.min_wpm) {
            player.alive = false;
            player.eliminated_stage = current_stage_;
        } else {
            player.survived_stages += 1;
        }
    }

    advance_stage();
}

bool SurvivalMode::tournament_finished() const {
    int alive_count = 0;
    for (const auto& pair : players_) {
        if (pair.second.alive) {
            alive_count++;
        }
    }
    return alive_count <= 1;
}

std::vector<SurvivalPlayerState> SurvivalMode::get_rankings() const {
    std::vector<SurvivalPlayerState> result;
    for (const auto& pair : players_) {
        result.push_back(pair.second);
    }

    std::sort(result.begin(), result.end(), [](const SurvivalPlayerState& a, const SurvivalPlayerState& b) {
        if (a.points != b.points) {
            return a.points > b.points;
        }
        if (a.survived_stages != b.survived_stages) {
            return a.survived_stages > b.survived_stages;
        }
        return a.name < b.name;
    });

    return result;
}

SurvivalStageRule SurvivalMode::rule_for_stage(int stage) const {
    if (stage <= 4) {
        return {25, 60, 60.0, 70.0};
    }
    if (stage <= 8) {
        return {50, 75, 70.0, 80.0};
    }
    if (stage <= 10) {
        return {75, 90, 80.0, 90.0};
    }
    int extra = stage - 10;
    return {75 + extra * 5, 90 + extra * 5, 80.0 + extra * 2.0, 90.0 + extra * 4.0};
}

void SurvivalMode::advance_stage() {
    current_stage_ += 1;
    if (tournament_finished()) {
        award_points();
        return;
    }
    start_stage();
}

void SurvivalMode::award_points() {
    std::vector<SurvivalPlayerState*> final_players;
    for (auto& pair : players_) {
        final_players.push_back(&pair.second);
    }

    std::sort(final_players.begin(), final_players.end(), [](const SurvivalPlayerState* a, const SurvivalPlayerState* b) {
        if (a->alive != b->alive) return a->alive > b->alive;
        if (a->survived_stages != b->survived_stages) return a->survived_stages > b->survived_stages;
        return a->name < b->name;
    });

    for (size_t i = 0; i < final_players.size(); ++i) {
        int points = 1;
        if (i == 0) points = 40;
        else if (i == 1) points = 20;
        else if (i == 2) points = 10;
        else if (i <= 4) points = 5;
        else if (i <= 7) points = 2;
        final_players[i]->points = points;
    }
}
