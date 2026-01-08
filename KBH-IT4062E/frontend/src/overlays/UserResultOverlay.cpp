#include "UserResultOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void UserResultOverlay::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;
    if (e.key.keysym.sym == SDLK_ESCAPE) app->router().pop();
}

void UserResultOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 170);

    SDL_SetRenderDrawColor(r, 25, 28, 38, 255);
    SDL_Rect panel{220, 140, 926, 480};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "User Result (stub)", 260, 180, UiTheme::White);

    std::string u = app->session().isLoggedIn() ? app->session().user().username : "(unknown)";
    drawText(r, f, "Account: " + u, 260, 240, UiTheme::Yellow);

    if (app->state().hasLastGameResult()) {
        const auto& res = app->state().getLastResult();
        drawText(r, f, "WPM: " + std::to_string(res.wpm), 260, 310, UiTheme::Warm);
        drawText(r, f, "Accuracy: " + std::to_string((int)res.accuracy) + "%", 260, 370, UiTheme::Warm);
        drawText(r, f, "Score: " + std::to_string(res.score), 260, 430, UiTheme::Warm);
    }

    drawText(r, f, "ESC: Close", 260, 520, UiTheme::White);
}
