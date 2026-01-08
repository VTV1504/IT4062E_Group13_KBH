#include "EnterRoomOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void EnterRoomOverlay::onEnter() {
    code.clear();
    SDL_StartTextInput();
}

void EnterRoomOverlay::onExit() {
    SDL_StopTextInput();
}

void EnterRoomOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT) {
        code += e.text.text;
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        if (!code.empty()) code.pop_back();
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        app->router().pop();              // close overlay
        app->router().change(RouteId::Lobby); // go lobby
    }
}

void EnterRoomOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 190);

    SDL_SetRenderDrawColor(r, 35, 35, 48, 255);
    SDL_Rect panel{260, 170, 846, 420};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Enter Room (stub)", 300, 210, UiTheme::White);

    drawText(r, f, "Code: " + (code.empty() ? std::string("_") : code), 300, 290, UiTheme::Warm);
    drawText(r, f, "ENTER: Join (dummy) -> Lobby", 300, 370, UiTheme::White);
    drawText(r, f, "ESC: Close", 300, 510, UiTheme::Yellow);
}
