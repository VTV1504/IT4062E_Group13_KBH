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
    SDL_Rect panel{160, 120, 1116, 640};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Guest Result", 200, 160, UiTheme::White);

    if (app->state().hasLastGameResult()) {
        const auto& res = app->state().getLastResult();
        drawText(r, f, "WPM: " + std::to_string(static_cast<int>(res.wpm)), 200, 240, UiTheme::Warm);
        drawText(r, f, "Accuracy: " + std::to_string(static_cast<int>(res.accuracy)) + "%", 200, 300, UiTheme::Warm);
        drawText(r, f, "Score: " + std::to_string(res.score), 200, 360, UiTheme::Warm);
    }

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    int y = 420;
    for (const auto& entry : app->state().ranking()) {
        std::string line = entry.name;
        if (entry.points > 0) {
            line += " | Points: " + std::to_string(entry.points);
            line += " | Stages: " + std::to_string(entry.survived_stages);
        } else {
            line += " | WPM: " + std::to_string(static_cast<int>(entry.wpm));
            line += " | Acc: " + std::to_string(static_cast<int>(entry.accuracy)) + "%";
        }
        drawText(r, sub, line, 200, y, UiTheme::White);
        y += 28;
        if (y > 700) break;
    }

    drawText(r, sub, "Press I: Sign in", 200, 700, UiTheme::Yellow);
    drawText(r, sub, "ESC: Close", 200, 730, UiTheme::White);
}
