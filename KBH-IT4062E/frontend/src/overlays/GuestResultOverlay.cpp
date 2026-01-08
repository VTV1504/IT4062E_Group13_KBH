#include "GuestResultOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void GuestResultOverlay::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
    } else if (e.key.keysym.sym == SDLK_i) {
        app->router().push(RouteId::SignInOverlay);
    }
}

void GuestResultOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 170);

    SDL_SetRenderDrawColor(r, 30, 30, 40, 255);
    SDL_Rect panel{220, 140, 926, 480};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Guest Result (stub)", 260, 180, UiTheme::White);

    if (app->state().hasLastGameResult()) {
        const auto& res = app->state().getLastResult();
        drawText(r, f, "WPM: " + std::to_string(res.wpm), 260, 260, UiTheme::Warm);
        drawText(r, f, "Accuracy: " + std::to_string((int)res.accuracy) + "%", 260, 320, UiTheme::Warm);
        drawText(r, f, "Score: " + std::to_string(res.score), 260, 380, UiTheme::Warm);
    }

    drawText(r, f, "Press I: Sign in to attach this result", 260, 470, UiTheme::Yellow);
    drawText(r, f, "ESC: Close", 260, 530, UiTheme::White);
}
