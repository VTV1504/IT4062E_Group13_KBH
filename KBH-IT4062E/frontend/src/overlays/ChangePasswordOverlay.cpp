#include "ChangePasswordOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void ChangePasswordOverlay::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;
    if (e.key.keysym.sym == SDLK_ESCAPE) app->router().pop();
}

void ChangePasswordOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 200);

    SDL_SetRenderDrawColor(r, 40, 40, 55, 255);
    SDL_Rect panel{300, 200, 766, 360};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Change Password (stub)", 340, 250, UiTheme::White);
    drawText(r, f, "ESC: Close", 340, 330, UiTheme::Yellow);
}
