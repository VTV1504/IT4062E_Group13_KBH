#pragma once
#include <string>
#include <vector>
#include "Models.h"

struct AuthResult {
    bool available = false;
    bool success = false;
    std::string type;
    std::string message;
    std::string username;
};

struct RoomResult {
    bool available = false;
    bool success = false;
    std::string message;
    std::string room_code;
    std::string mode;
    bool is_host = false;
};

class AppState {
public:
    void setPendingMode(GameMode m) { pendingMode = m; }
    GameMode getPendingMode() const { return pendingMode; }

    void setLastResult(const GameResult& r) { lastResult = r; hasResult = true; }
    bool hasLastGameResult() const { return hasResult; }
    const GameResult& getLastResult() const { return lastResult; }

    void setConnectionStatus(bool ok, const std::string& msg);
    bool isConnected() const { return connected; }
    const std::string& connectionMessage() const { return connection_msg; }

    void setNotice(const std::string& msg) { notice = msg; }
    const std::string& getNotice() const { return notice; }

    void setAuthResult(const AuthResult& result);
    bool takeAuthResult(AuthResult& out);

    void setRoomResult(const RoomResult& result);
    bool takeRoomResult(RoomResult& out);

    void setRoomInfo(const std::string& code, const std::string& mode, bool host);
    const std::string& roomCode() const { return room_code; }
    const std::string& roomMode() const { return room_mode; }
    bool isRoomHost() const { return room_host; }
    bool isReady() const { return room_ready; }
    void setReady(bool ready) { room_ready = ready; }
    void setAllReady(bool ready) { room_all_ready = ready; }
    bool allReady() const { return room_all_ready; }

    void setGamePayload(GameMode mode, const std::string& text, int duration, int stage, const std::string& difficulty);
    GameMode activeMode() const { return active_mode; }
    const std::string& gameText() const { return game_text; }
    int gameDuration() const { return game_duration; }
    int gameStage() const { return game_stage; }
    const std::string& gameDifficulty() const { return game_difficulty; }

    void setRanking(const std::vector<RankingEntry>& entries);
    const std::vector<RankingEntry>& ranking() const { return ranking_entries; }

    void setTrainingLeaderboard(const std::vector<LeaderboardEntry>& entries, int rank);
    void setSurvivalLeaderboard(const std::vector<LeaderboardEntry>& entries, int rank);
    const std::vector<LeaderboardEntry>& trainingLeaderboard() const { return training_entries; }
    const std::vector<LeaderboardEntry>& survivalLeaderboard() const { return survival_entries; }
    int trainingRank() const { return training_rank; }
    int survivalRank() const { return survival_rank; }

private:
    GameMode pendingMode = GameMode::Training;
    bool hasResult = false;
    GameResult lastResult{};

    bool connected = false;
    std::string connection_msg;
    std::string notice;

    AuthResult authResult;
    RoomResult roomResult;

    std::string room_code;
    std::string room_mode;
    bool room_host = false;
    bool room_ready = false;
    bool room_all_ready = false;

    GameMode active_mode = GameMode::Training;
    std::string game_text;
    int game_duration = 0;
    int game_stage = 0;
    std::string game_difficulty;

    std::vector<RankingEntry> ranking_entries;
    std::vector<LeaderboardEntry> training_entries;
    std::vector<LeaderboardEntry> survival_entries;
    int training_rank = 0;
    int survival_rank = 0;
};
