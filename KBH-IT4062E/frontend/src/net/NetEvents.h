#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Base event type
enum class NetEventType {
    Hello,
    TimeSync,
    RoomState,
    GameInit,
    GameState,
    GameEnd,
    Error,
    Info
};

struct NetEvent {
    NetEventType type;
    virtual ~NetEvent() = default;
};

// hello event
struct HelloEvent : NetEvent {
    HelloEvent() { type = NetEventType::Hello; }
    int client_id = 0;
    int64_t server_time_ms = 0;
};

// time_sync reply
struct TimeSyncEvent : NetEvent {
    TimeSyncEvent() { type = NetEventType::TimeSync; }
    int client_id = 0;
    int64_t server_time_ms = 0;
    int64_t client_time_ms = 0;
};

// room_state
struct RoomSlotData {
    int slot_idx = 0;
    bool occupied = false;
    int client_id = 0;
    std::string display_name;
    bool is_host = false;
    bool is_ready = false;
    int knight_idx = 0;
};

struct RoomStateEvent : NetEvent {
    RoomStateEvent() { type = NetEventType::RoomState; }
    std::string room_id;
    bool is_private = false;
    int max_players = 8;
    int self_client_id = 0;
    int host_slot_idx = 0;
    bool all_ready = false;
    bool can_start = false;
    RoomSlotData slots[8];
};

// game_init
struct GamePlayerData {
    int slot_idx = 0;
    int client_id = 0;
    std::string display_name;
};

struct GameInitEvent : NetEvent {
    GameInitEvent() { type = NetEventType::GameInit; }
    std::string room_id;
    int64_t server_start_ms = 0;
    int duration_ms = 50000;
    std::string paragraph;
    int total_words = 0;
    std::vector<GamePlayerData> players;
};

// game_state
struct GamePlayerState {
    int slot_idx = 0;
    bool occupied = false;
    int word_idx = 0;
    int64_t latest_time_ms = 0;
    double progress = 0.0;
    double wpm = 0.0;
    double accuracy = 0.0;
};

struct GameStateEvent : NetEvent {
    GameStateEvent() { type = NetEventType::GameState; }
    std::string room_id;
    int64_t server_now_ms = 0;
    int duration_ms = 50000;
    bool ended = false;
    GamePlayerState players[8];
};

// game_end
struct RankingData {
    int rank = 0;
    int slot_idx = 0;
    int client_id = 0;
    std::string display_name;
    int word_idx = 0;
    int64_t latest_time_ms = 0;
    double wpm = 0.0;
    double accuracy = 0.0;
};

struct GameEndEvent : NetEvent {
    GameEndEvent() { type = NetEventType::GameEnd; }
    std::string room_id;
    std::string reason; // "timeout" or "all_finished"
    std::vector<RankingData> rankings;
};

// error
struct ErrorEvent : NetEvent {
    ErrorEvent() { type = NetEventType::Error; }
    std::string code;
    std::string message;
};

// info
struct InfoEvent : NetEvent {
    InfoEvent() { type = NetEventType::Info; }
    std::string code;
    std::string message;
};
