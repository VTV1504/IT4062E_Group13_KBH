#include "GameScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <jsoncpp/json/json.h>
#include <algorithm>

void GameScreen::onEnter() {
    mode = app->state().activeMode();
    targetText = app->state().gameText();
    typedText.clear();
    durationSeconds = app->state().gameDuration();
    startTicks = SDL_GetTicks();
    helperMessage.clear();
    SDL_StartTextInput();
}

void GameScreen::onExit() {
    SDL_StopTextInput();
}

void GameScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT) {
        std::string input = e.text.text;
        for (char c : input) {
            typedText.push_back(c);
            Json::Value payload;
            std::string s(1, c);
            payload["key"] = s;
            app->network().sendCommand("KEY_INPUT", payload);
        }
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().change(RouteId::Title);
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        if (!typedText.empty()) {
            typedText.pop_back();
            helperMessage = "Backspace not sent to server.";
        }
        return;
    }
}

void GameScreen::update(float) {
    if (durationSeconds == 0) return;
    Uint32 elapsed = (SDL_GetTicks() - startTicks) / 1000;
    if (elapsed >= static_cast<Uint32>(durationSeconds)) {
        helperMessage = "Waiting for server result...";
    }
}

void GameScreen::render(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 12, 12, 18, 255);
    SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
    SDL_RenderFillRect(r, &dst);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 38);
    std::string modeLabel = (mode == GameMode::Training) ? "Self Training" : (mode == GameMode::Arena ? "Arena" : "Survival");
    drawText(r, f, "Mode: " + modeLabel, 80, 60, UiTheme::Warm);

    if (mode == GameMode::Arena && !app->state().gameDifficulty().empty()) {
        drawText(r, f, "Difficulty: " + app->state().gameDifficulty(), 80, 110, UiTheme::Yellow);
    }

    if (mode == GameMode::Survival) {
        drawText(r, f, "Stage: " + std::to_string(app->state().gameStage()), 80, 110, UiTheme::Yellow);
    }

    Uint32 elapsed = (SDL_GetTicks() - startTicks) / 1000;
    int remaining = durationSeconds > 0 ? std::max(0, durationSeconds - static_cast<int>(elapsed)) : 0;
    drawText(r, f, "Time left: " + std::to_string(remaining) + "s", 80, 160, UiTheme::White);

    TTF_Font* bodyFont = app->resources().font(UiTheme::SubFontPath, 26);
    drawText(r, bodyFont, "Text:", 80, 230, UiTheme::Warm);
    drawText(r, bodyFont, targetText, 80, 270, UiTheme::White);

    drawText(r, bodyFont, "Your input:", 80, 420, UiTheme::Warm);
    drawText(r, bodyFont, typedText, 80, 460, UiTheme::Yellow);

    if (!helperMessage.empty()) {
        drawText(r, bodyFont, helperMessage, 80, 540, SDL_Color{255, 180, 100, 255});
    }

    drawText(r, bodyFont, "ESC: Quit to Title", 80, 900, UiTheme::Warm);
}
