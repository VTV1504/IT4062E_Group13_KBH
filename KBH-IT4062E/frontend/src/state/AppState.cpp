#include "AppState.h"

void AppState::setConnectionStatus(bool ok, const std::string& msg) {
    connected = ok;
    connection_msg = msg;
}

void AppState::setAuthResult(const AuthResult& result) {
    authResult = result;
    authResult.available = true;
}

bool AppState::takeAuthResult(AuthResult& out) {
    if (!authResult.available) return false;
    out = authResult;
    authResult.available = false;
    return true;
}

void AppState::setRoomResult(const RoomResult& result) {
    roomResult = result;
    roomResult.available = true;
}

bool AppState::takeRoomResult(RoomResult& out) {
    if (!roomResult.available) return false;
    out = roomResult;
    roomResult.available = false;
    return true;
}

void AppState::setRoomInfo(const std::string& code, const std::string& mode, bool host) {
    room_code = code;
    room_mode = mode;
    room_host = host;
    room_ready = false;
    room_all_ready = false;
}

void AppState::setGamePayload(GameMode mode, const std::string& text, int duration, int stage, const std::string& difficulty) {
    active_mode = mode;
    game_text = text;
    game_duration = duration;
    game_stage = stage;
    game_difficulty = difficulty;
}

void AppState::setRanking(const std::vector<RankingEntry>& entries) {
    ranking_entries = entries;
}

void AppState::setTrainingLeaderboard(const std::vector<LeaderboardEntry>& entries, int rank) {
    training_entries = entries;
    training_rank = rank;
}

void AppState::setSurvivalLeaderboard(const std::vector<LeaderboardEntry>& entries, int rank) {
    survival_entries = entries;
    survival_rank = rank;
}
