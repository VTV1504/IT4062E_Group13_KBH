#include "room.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

Room::Room(const std::string& id, Database* db)
    : id_(id), db_(db)
{
    // Get random paragraph
    if (db_) {
        paragraph_ = db_->get_random_paragraph("en");
    } else {
        paragraph_ = "The quick brown fox jumps over the lazy dog";
    }
    
    // Count words
    std::istringstream iss(paragraph_);
    std::string word;
    while (iss >> word) {
        paragraph_words_.push_back(word);
        total_words_++;
    }
}

Room::~Room() {}

bool Room::add_player(int fd, int client_id, const std::string& display_name) {
    // Find first empty slot
    for (int i = 0; i < 8; i++) {
        if (!slots_[i].occupied) {
            slots_[i].occupied = true;
            slots_[i].client_fd = fd;
            slots_[i].client_id = client_id;
            slots_[i].display_name = display_name;
            slots_[i].is_ready = false;
            
            // If this is the first player, they become host
            if (host_slot_idx_ == -1) {
                host_slot_idx_ = i;
            }
            
            return true;
        }
    }
    
    return false; // Room full
}

void Room::remove_player(int fd) {
    int slot_idx = find_slot_by_fd(fd);
    if (slot_idx == -1) return;
    
    slots_[slot_idx].occupied = false;
    slots_[slot_idx].client_fd = -1;
    slots_[slot_idx].client_id = 0;
    slots_[slot_idx].display_name.clear();
    slots_[slot_idx].is_ready = false;
    
    player_metrics_.erase(fd);
    
    // Recalculate host if needed
    recalculate_host();
}

int Room::player_count() const {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied) count++;
    }
    return count;
}

bool Room::is_host(int fd) const {
    if (host_slot_idx_ == -1) return false;
    return slots_[host_slot_idx_].client_fd == fd;
}

void Room::set_ready(int fd, bool ready) {
    int slot_idx = find_slot_by_fd(fd);
    if (slot_idx != -1) {
        slots_[slot_idx].is_ready = ready;
    }
}

bool Room::all_ready() const {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied) {
            if (!slots_[i].is_ready) return false;
            count++;
        }
    }
    return count > 0;
}

bool Room::can_start() const {
    if (host_slot_idx_ == -1) return false;
    if (player_count() < 2) return false;
    return all_ready();
}

void Room::update_display_name(int fd, const std::string& name) {
    int slot_idx = find_slot_by_fd(fd);
    if (slot_idx != -1) {
        slots_[slot_idx].display_name = name;
    }
}

void Room::start_game(int64_t start_time, int duration) {
    game_started_ = true;
    game_start_time_ = start_time;
    game_duration_ms_ = duration;
    
    // Initialize metrics for all players
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied) {
            player_metrics_[slots_[i].client_fd] = PlayerMetrics{};
        }
    }
}

void Room::end_game() {
    game_started_ = false;
    
    // Unready all players
    for (int i = 0; i < 8; i++) {
        slots_[i].is_ready = false;
    }
}

bool Room::is_game_ended(int64_t current_time) const {
    if (!game_started_) return false;
    
    // Check timeout
    if (current_time - game_start_time_ >= game_duration_ms_) {
        return true;
    }
    
    // Check if all finished
    return all_finished();
}

void Room::process_input(int fd, int word_idx, const Json::Value& char_events) {
    auto& metrics = player_metrics_[fd];
    
    // Update word_idx
    if (word_idx >= metrics.word_idx) {
        metrics.word_idx = word_idx + 1; // Next word index
    }
    
    // Get the target word for accuracy calculation
    std::string target_word = "";
    if (word_idx >= 0 && word_idx < (int)paragraph_words_.size()) {
        target_word = paragraph_words_[word_idx];
    }
    
    // Calculate metrics from char_events
    int correct_chars = 0;
    int total_chars = 0;
    std::string typed_word = "";
    
    for (const auto& event : char_events) {
        std::string event_type = event["type"].asString();
        
        if (event_type == "char") {
            std::string ch = event["char"].asString();
            if (!ch.empty()) {
                typed_word += ch[0];
                total_chars++;
            }
        } else if (event_type == "backspace") {
            if (!typed_word.empty()) {
                typed_word.pop_back();
            }
        }
        
        if (event.isMember("time_ms")) {
            metrics.latest_time_ms = event["time_ms"].asInt64();
        }
    }
    
    // Calculate correct chars by comparing typed_word with target_word
    size_t min_len = std::min(typed_word.length(), target_word.length());
    for (size_t i = 0; i < min_len; i++) {
        if (typed_word[i] == target_word[i]) {
            correct_chars++;
        }
    }
    // If lengths don't match, remaining chars are wrong
    // correct_chars already counted for matching portion
    
    // Update cumulative stats
    metrics.total_correct_chars += correct_chars;
    metrics.total_chars_typed += total_chars;
    
    // Calculate progress
    metrics.progress = (double)metrics.word_idx / total_words_;
    
    // Calculate WPM (based on total correct chars typed so far)
    if (metrics.latest_time_ms > game_start_time_) {
        double elapsed_ms = metrics.latest_time_ms - game_start_time_;
        double elapsed_minutes = elapsed_ms / 60000.0;
        if (elapsed_minutes > 0) {
            // WPM = (total correct chars / 5) / elapsed_minutes
            metrics.wpm = (metrics.total_correct_chars / 5.0) / elapsed_minutes;
        }
    }
    
    // Calculate accuracy (cumulative)
    if (metrics.total_chars_typed > 0) {
        metrics.accuracy = (metrics.total_correct_chars * 100.0) / metrics.total_chars_typed;
    } else {
        metrics.accuracy = 100.0;
    }
}

PlayerMetrics Room::get_player_metrics(int fd) const {
    auto it = player_metrics_.find(fd);
    if (it != player_metrics_.end()) {
        return it->second;
    }
    return PlayerMetrics{};
}

bool Room::all_finished() const {
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied) {
            auto it = player_metrics_.find(slots_[i].client_fd);
            if (it == player_metrics_.end() || it->second.word_idx < total_words_) {
                return false;
            }
        }
    }
    return true;
}

std::vector<RankingEntry> Room::get_rankings() const {
    std::vector<RankingEntry> rankings;
    
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied) {
            RankingEntry entry;
            entry.slot_idx = i;
            entry.client_id = slots_[i].client_id;
            entry.display_name = slots_[i].display_name;
            
            auto metrics = get_player_metrics(slots_[i].client_fd);
            entry.word_idx = metrics.word_idx;
            entry.latest_time_ms = metrics.latest_time_ms;
            entry.wpm = metrics.wpm;
            entry.accuracy = metrics.accuracy;
            
            rankings.push_back(entry);
        }
    }
    
    // Sort by word_idx (desc), then by time (asc)
    std::sort(rankings.begin(), rankings.end(), [](const RankingEntry& a, const RankingEntry& b) {
        if (a.word_idx != b.word_idx) return a.word_idx > b.word_idx;
        return a.latest_time_ms < b.latest_time_ms;
    });
    
    // Assign ranks
    for (size_t i = 0; i < rankings.size(); i++) {
        rankings[i].rank = i + 1;
    }
    
    return rankings;
}

void Room::recalculate_host() {
    // Find the lowest occupied slot
    host_slot_idx_ = -1;
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied) {
            host_slot_idx_ = i;
            break;
        }
    }
}

int Room::find_slot_by_fd(int fd) const {
    for (int i = 0; i < 8; i++) {
        if (slots_[i].occupied && slots_[i].client_fd == fd) {
            return i;
        }
    }
    return -1;
}
