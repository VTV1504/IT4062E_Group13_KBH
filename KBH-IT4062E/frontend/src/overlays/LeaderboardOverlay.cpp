#include "LeaderboardOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void LeaderboardOverlay::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;
    if (e.key.keysym.sym == SDLK_ESCAPE) app->router().pop();
}

void LeaderboardOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 180);

    SDL_SetRenderDrawColor(r, 28, 30, 42, 255);
    SDL_Rect panel{200, 120, 966, 520};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Leaderboard (stub)", 240, 170, UiTheme::White);
    drawText(r, f, "ESC: Close", 240, 240, UiTheme::Yellow);
}
