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
    SDL_Rect panel{160, 120, 1116, 640};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Result", 200, 160, UiTheme::White);

    std::string u = app->session().isLoggedIn() ? app->session().user().username : "(unknown)";
    drawText(r, f, "Account: " + u, 200, 220, UiTheme::Yellow);

    if (app->state().hasLastGameResult()) {
        const auto& res = app->state().getLastResult();
        drawText(r, f, "WPM: " + std::to_string(static_cast<int>(res.wpm)), 200, 280, UiTheme::Warm);
        drawText(r, f, "Accuracy: " + std::to_string(static_cast<int>(res.accuracy)) + "%", 200, 340, UiTheme::Warm);
        drawText(r, f, "Score: " + std::to_string(res.score), 200, 400, UiTheme::Warm);
    }

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    int y = 460;
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

    drawText(r, sub, "ESC: Close", 200, 720, UiTheme::White);
}
