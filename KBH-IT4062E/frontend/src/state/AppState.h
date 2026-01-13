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

private:
    GameMode pendingMode = GameMode::Training;
    bool hasResult = false;
    GameResult lastResult{};
    
    bool hasRoomState = false;
    RoomStateEvent roomState;
    
    bool hasGameInit_ = false;
    GameInitEvent gameInit;
    
    bool hasGameState_ = false;
    GameStateEvent gameState;
    
    bool hasGameEnd_ = false;
    GameEndEvent gameEnd;
};
