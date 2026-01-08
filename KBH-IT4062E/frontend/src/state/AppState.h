#pragma once
#include "Models.h"

class AppState {
public:
    void setPendingMode(GameMode m) { pendingMode = m; }
    GameMode getPendingMode() const { return pendingMode; }

    void setLastResult(const GameResult& r) { lastResult = r; hasResult = true; }
    bool hasLastGameResult() const { return hasResult; }
    const GameResult& getLastResult() const { return lastResult; }

private:
    GameMode pendingMode = GameMode::Training;
    bool hasResult = false;
    GameResult lastResult{};
};
