#include "GameScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <cstdlib>

void GameScreen::onEnter() {
    mode = app->state().getPendingMode();
}

void GameScreen::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().change(RouteId::Title);
        return;
    }

    if (e.key.keysym.sym == SDLK_SPACE) {
        GameResult r;
        r.mode = mode;
        r.wpm = 40 + (std::rand() % 80);
        r.accuracy = 80.0f + (std::rand() % 21);
        r.score = r.wpm * 10;

        app->state().setLastResult(r);

        if (app->session().isLoggedIn()) app->router().push(RouteId::UserResultOverlay);
        else app->router().push(RouteId::GuestResultOverlay);
    }
}

void GameScreen::render(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 12, 12, 18, 255);
    SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
    SDL_RenderFillRect(r, &dst);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Game Screen (stub)", 80, 80, UiTheme::White);

    if (mode == GameMode::Training) drawText(r, f, "Mode: Training", 80, 150, UiTheme::Warm);
    else drawText(r, f, "Mode: Arena", 80, 150, UiTheme::Warm);

    drawText(r, f, "Press SPACE to finish and show Result overlay", 80, 220, UiTheme::White);
    drawText(r, f, "ESC: Back to Title", 80, 280, UiTheme::Yellow);
}
