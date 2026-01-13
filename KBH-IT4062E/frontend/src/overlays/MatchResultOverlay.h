#pragma once
#include "../core/View.h"
#include "../net/NetEvents.h"
#include <SDL_ttf.h>
#include <vector>

class MatchResultOverlay : public View {
public:
    void onEnter() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    std::vector<RankingData> rankings;
    
    // Button areas
    SDL_Rect exitMenuBtn;
    SDL_Rect returnLobbyBtn;
    
    int hoveredBtn = -1; // 0=exit, 1=return
    
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void renderRankingRow(SDL_Renderer* r, int rowIndex, const RankingData& data, int y, bool isFirst);
    void handleExitMenu();
    void handleReturnLobby();
};
