#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <jsoncpp/json/json.h>
#include "../database/database.h"

struct RoomSlot {
    bool occupied = false;
    int client_fd = -1;
    int client_id = 0;
    std::string display_name;
    bool is_ready = false;
};

struct PlayerMetrics {
    int word_idx = 0;
    int64_t latest_time_ms = 0;
    double progress = 0.0;
    double wpm = 0.0;
    double accuracy = 0.0;
    
    // Cumulative stats for accurate WPM/accuracy calculation
    int total_correct_chars = 0;
    int total_chars_typed = 0;
};

struct RankingEntry {
    int rank = 0;
    int slot_idx = 0;
    int client_id = 0;
    std::string display_name;
    int word_idx = 0;
    int64_t latest_time_ms = 0;
    double wpm = 0.0;
    double accuracy = 0.0;
};

class Room {
public:
    Room(const std::string& id, Database* db);
    ~Room();

    const std::string& id() const { return id_; }
    
    // Slot management (8 fixed slots)
    bool add_player(int fd, int client_id, const std::string& display_name);
    void remove_player(int fd);
    const RoomSlot& get_slot(int idx) const { return slots_[idx]; }
    int player_count() const;
    
    // Host management
    int host_slot_idx() const { return host_slot_idx_; }
    bool is_host(int fd) const;
    
    // Ready system
    void set_ready(int fd, bool ready);
    bool all_ready() const;
    bool can_start() const;
    
    // Privacy
    bool is_private() const { return is_private_; }
    void set_private(bool priv) { is_private_ = priv; }
    
    // Display name update
    void update_display_name(int fd, const std::string& name);
    
    // Game management
    void start_game(int64_t start_time, int duration);
    void end_game();
    bool is_game_started() const { return game_started_; }
    bool is_game_ended(int64_t current_time) const;
    int64_t game_start_time() const { return game_start_time_; }
    int game_duration() const { return game_duration_ms_; }
    
    // Paragraph
    const std::string& paragraph() const { return paragraph_; }
    int total_words() const { return total_words_; }
    
    // Input processing
    void process_input(int fd, int word_idx, const Json::Value& char_events);
    PlayerMetrics get_player_metrics(int fd) const;
    bool all_finished() const;
    std::vector<RankingEntry> get_rankings() const;

private:
    void recalculate_host();
    int find_slot_by_fd(int fd) const;
    
    std::string id_;
    RoomSlot slots_[8];
    int host_slot_idx_ = -1;
    bool is_private_ = false;
    
    // Game state
    bool game_started_ = false;
    int64_t game_start_time_ = 0;
    int game_duration_ms_ = 50000;
    
    // Paragraph
    std::string paragraph_;
    int total_words_ = 0;    std::vector<std::string> paragraph_words_;    
    // Player metrics
    std::unordered_map<int, PlayerMetrics> player_metrics_;
    
    Database* db_;
};

#endif
