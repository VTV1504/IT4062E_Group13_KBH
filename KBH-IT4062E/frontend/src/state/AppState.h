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

private:
    GameMode pendingMode = GameMode::Training;
    bool hasResult = false;
    GameResult lastResult{};
    
    bool hasRoomState = false;
    RoomStateEvent roomState;
};
