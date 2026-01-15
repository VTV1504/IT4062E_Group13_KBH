#pragma once
#include "../core/View.h"
#include "../net/NetEvents.h"
#include <SDL_rect.h>

class UserResultOverlay : public View {
public:
    void onEnter() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    RankingData playerResult;
    SDL_Rect exitMenuBtn;
    SDL_Rect tryAgainBtn;
    int hoveredBtn = -1;  // 0=exit, 1=tryagain
    
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void handleExitMenu();
    void handleTryAgain();
};
