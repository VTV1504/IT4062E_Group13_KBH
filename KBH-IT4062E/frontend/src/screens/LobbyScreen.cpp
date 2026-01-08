#include "LobbyScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void LobbyScreen::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().change(RouteId::Title);
    } else if (e.key.keysym.sym == SDLK_s) {
        app->state().setPendingMode(GameMode::Arena);
        app->router().change(RouteId::Game);
    } else if (e.key.keysym.sym == SDLK_t) {
        app->state().setPendingMode(GameMode::Training);
        app->router().change(RouteId::Game);
    }
}

void LobbyScreen::render(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 20, 25, 35, 255);
    SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
    SDL_RenderFillRect(r, &dst);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Lobby Screen (stub)", 80, 80, UiTheme::White);
    drawText(r, f, "Press S: Start Arena", 80, 150, UiTheme::Warm);
    drawText(r, f, "Press T: Training", 80, 210, UiTheme::Warm);
    drawText(r, f, "ESC: Back to Title", 80, 270, UiTheme::Yellow);
}
