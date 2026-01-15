#pragma once
#include "Models.h"
#include "../net/NetEvents.h"

class AppState {
public:
    void setPendingMode(GameMode m) { pendingMode = m; }
    GameMode getPendingMode() const { return pendingMode; }

    void setLastResult(const GameResult& r) { lastResult = r; hasResult = true; }
    bool hasLastGameResult() const { return hasResult; }
    const GameResult& getLastResult() const { return lastResult; }
    
    // User authentication
    void setUser(int64_t id, const std::string& name) { 
        userId = id; 
        username = name; 
        isAuthenticated = true; 
    }
    void clearUser() { 
        userId = -1; 
        username.clear(); 
        isAuthenticated = false; 
    }
    bool isUserAuthenticated() const { return isAuthenticated; }
    int64_t getUserId() const { return userId; }
    const std::string& getUsername() const { return username; }
    
    // Room state
    void setRoomState(const RoomStateEvent& rs) { roomState = rs; hasRoomState = true; }
    bool hasRoom() const { return hasRoomState; }
    const RoomStateEvent& getRoomState() const { return roomState; }
    void clearRoomState() { hasRoomState = false; }
    
    // Game state
    void setGameInit(const GameInitEvent& gi) { gameInit = gi; hasGameInit_ = true; }
    bool hasGameInit() const { return hasGameInit_; }
    const GameInitEvent& getGameInit() const { return gameInit; }
    void clearGameInit() { hasGameInit_ = false; }
    
    void setGameState(const GameStateEvent& gs) { gameState = gs; hasGameState_ = true; }
    bool hasGameState() const { return hasGameState_; }
    const GameStateEvent& getGameState() const { return gameState; }
    void clearGameState() { hasGameState_ = false; }
    
    void setGameEnd(const GameEndEvent& ge) { gameEnd = ge; hasGameEnd_ = true; }
    bool hasGameEnd() const { return hasGameEnd_; }
    const GameEndEvent& getGameEnd() const { return gameEnd; }
    void clearGameEnd() { hasGameEnd_ = false; }
    
    // Leaderboard
    void setLeaderboard(const LeaderboardResponseEvent& lb) { 
        if (leaderboard) delete leaderboard;
        leaderboard = new LeaderboardResponseEvent(lb); 
        hasLeaderboard_ = true; 
    }
    bool hasLeaderboard() const { return hasLeaderboard_; }
    const LeaderboardResponseEvent& getLeaderboard() const { return *leaderboard; }
    void clearLeaderboard() { 
        if (leaderboard) {
            delete leaderboard;
            leaderboard = nullptr;
        }
        hasLeaderboard_ = false; 
    }
    
    // Auto-start training flag (for Try Again)
    void setAutoStartTraining(bool val) { autoStartTraining_ = val; }
    bool shouldAutoStartTraining() const { return autoStartTraining_; }

private:
    GameMode pendingMode = GameMode::Training;
    bool hasResult = false;
    GameResult lastResult{};
    
    // User authentication
    bool isAuthenticated = false;
    int64_t userId = -1;
    std::string username;
    
    bool hasRoomState = false;
    RoomStateEvent roomState;
    
    bool hasGameInit_ = false;
    GameInitEvent gameInit;
    
    bool hasGameState_ = false;
    GameStateEvent gameState;
    
    bool hasGameEnd_ = false;
    GameEndEvent gameEnd;
    
    bool hasLeaderboard_ = false;
    LeaderboardResponseEvent* leaderboard = nullptr;
    
    bool autoStartTraining_ = false;
};
