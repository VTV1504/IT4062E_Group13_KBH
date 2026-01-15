#pragma once
#include "../core/View.h"
#include "../net/NetEvents.h"
#include <SDL_rect.h>

class GuestResultOverlay : public View {
public:
    void onEnter() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    RankingData playerResult;
    SDL_Rect exitMenuBtn;
    SDL_Rect loginBtn;
    int hoveredBtn = -1;  // 0=exit, 1=login
    
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void handleExitMenu();
    void handleLoginToSave();
};
