#pragma once
#include "../core/View.h"
#include "../net/NetEvents.h"  // For LeaderboardEntry
#include <vector>
#include <string>

class LeaderboardOverlay : public View {
public:
    LeaderboardOverlay();
    void onEnter() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    
    SDL_Rect panelRect;  // For click outside detection
    int scrollOffset;
    int maxScroll;
};
