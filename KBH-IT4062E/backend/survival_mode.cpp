#include "survival_mode.h"

#include <algorithm>

SurvivalMode::SurvivalMode() {}
SurvivalMode::~SurvivalMode() {}

void SurvivalMode::start() {
    if ((int)players_.size() < min_players_) {
        std::cout << "Cannot start Survival Mode: need at least " << min_players_ << " players.\n";
        return;
    }
    std::cout << "Starting Survival Mode with " << players_.size() << " players, stage " << current_stage_ << "...\n";
}

void SurvivalMode::process_input(const std::string& input) {
    std::cout << "SurvivalMode::process_input called (use process_player_result instead)\n";
}

void SurvivalMode::add_player(const std::string& player_id) {
    if (players_.size() >= 10) return;
    SurvivalPlayer p;
    p.id = player_id;
    players_.push_back(p);
}

static void stage_requirements(int stage, int &word_count, double &min_accuracy, double &min_wpm) {
    if (stage <= 3) {
        word_count = 30; min_accuracy = 40.0; min_wpm = 20.0;
    } else if (stage <= 6) {
        word_count = 50; min_accuracy = 50.0; min_wpm = 35.0;
    } else if (stage <= 8) {
        word_count = 70; min_accuracy = 60.0; min_wpm = 45.0;
    } else {
        int extra = stage - 8;
        word_count = 70;
        min_accuracy = std::min(90.0, 60.0 + 5.0 * extra);
        min_wpm = 45.0 + 7.0 * extra;
    }
}

void SurvivalMode::process_player_result(const std::string& player_id, const std::string& typed, double time_seconds) {
    // find player
    for (auto &p : players_) {
        if (p.id == player_id && p.alive) {
            auto now = std::chrono::steady_clock::now();
            auto orig_start = start_time_;
            auto orig_end = end_time_;
            end_time_ = now;
            start_time_ = now - std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(time_seconds));
            p.last_result = compute_result(typed);
            // restore
            start_time_ = orig_start;
            end_time_ = orig_end;

            // check requirements for current stage
            int wc; double req_acc; double req_wpm;
            stage_requirements(current_stage_, wc, req_acc, req_wpm);
            if (p.last_result.accuracy < req_acc || p.last_result.wpm < req_wpm) {
                p.alive = false;
                p.eliminated_stage = current_stage_;
            } else {
                p.survived_stages += 1;
            }
            break;
        }
    }

    // if <=3 alive, end session
    int alive_count = 0;
    for (auto &pp : players_) if (pp.alive) ++alive_count;
    if (alive_count <= 3) {
        // assign points
        std::vector<SurvivalPlayer*> survivors;
        for (auto &pp : players_) if (pp.alive) survivors.push_back(&pp);
        std::sort(survivors.begin(), survivors.end(), [](const SurvivalPlayer* a, const SurvivalPlayer* b){
            if (a->last_result.wpm != b->last_result.wpm) return a->last_result.wpm > b->last_result.wpm;
            return a->last_result.accuracy > b->last_result.accuracy;
        });
        // assign placement points
        if (survivors.size() > 0) survivors[0]->points += 30;
        if (survivors.size() > 1) survivors[1]->points += 24;
        if (survivors.size() > 2) survivors[2]->points += 12;

        // bonus for stages survived beyond 8
        for (auto &pp : players_) {
            if (pp.survived_stages > 8) pp.points += 4 * (pp.survived_stages - 8);
        }

        end();
    }
}

void SurvivalMode::advance_stage() {
    current_stage_ += 1;
    // regenerate target text based on stage
    int wc; double acc; double wpm;
    stage_requirements(current_stage_, wc, acc, wpm);
    std::string text;
    for (int i = 0; i < wc; ++i) {
        if (i) text += ' ';
        text += "word";
    }
    set_target_text(text);
}

void SurvivalMode::end() {
    std::cout << "Ending Survival Mode...\n";
    // show final standings by points
    std::sort(players_.begin(), players_.end(), [](const SurvivalPlayer &a, const SurvivalPlayer &b){
        return a.points > b.points;
    });
}

void SurvivalMode::display_results() {
    std::cout << "Survival Results (by points):\n";
    int rank = 1;
    for (const auto &p : players_) {
        std::cout << rank++ << ". " << p.id << " - Points: " << p.points << " | Survived stages: " << p.survived_stages << "\n";
    }
}

