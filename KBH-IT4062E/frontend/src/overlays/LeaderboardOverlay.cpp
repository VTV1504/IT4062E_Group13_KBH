#include "LeaderboardOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <jsoncpp/json/json.h>

void LeaderboardOverlay::onEnter() {
    statusMessage.clear();
    showTraining = true;
    app->network().sendCommand("SELF_TRAINING_LEADERBOARD", Json::Value(Json::objectValue));
}

void LeaderboardOverlay::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;
    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
        return;
    }
    if (e.key.keysym.sym == SDLK_1) {
        showTraining = true;
        app->network().sendCommand("SELF_TRAINING_LEADERBOARD", Json::Value(Json::objectValue));
        return;
    }
    if (e.key.keysym.sym == SDLK_2) {
        showTraining = false;
        app->network().sendCommand("SURVIVAL_LEADERBOARD", Json::Value(Json::objectValue));
        return;
    }
}

void LeaderboardOverlay::update(float) {
    if (!app->state().getNotice().empty()) {
        statusMessage = app->state().getNotice();
    }
}

void LeaderboardOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 180);

    SDL_SetRenderDrawColor(r, 28, 30, 42, 255);
    SDL_Rect panel{160, 100, 1116, 650};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Leaderboard", 200, 140, UiTheme::White);

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    drawText(r, sub, "1: Self Training   2: Survival", 200, 200, UiTheme::Warm);

    const auto& entries = showTraining ? app->state().trainingLeaderboard() : app->state().survivalLeaderboard();
    int rank = showTraining ? app->state().trainingRank() : app->state().survivalRank();

    int y = 260;
    int index = 1;
    for (const auto& entry : entries) {
        std::string line = std::to_string(index) + ". " + entry.username;
        if (showTraining) {
            line += " | WPM: " + std::to_string(static_cast<int>(entry.value_a));
            line += " | Acc: " + std::to_string(static_cast<int>(entry.value_b)) + "%";
        } else {
            line += " | Points: " + std::to_string(entry.points);
            line += " | Rooms: " + std::to_string(entry.rooms);
        }
        drawText(r, sub, line, 200, y, UiTheme::White);
        y += 30;
        index++;
        if (index > 15) break;
    }

    if (rank > 0) {
        drawText(r, sub, "Your rank: " + std::to_string(rank), 200, 620, UiTheme::Yellow);
    }

    if (!statusMessage.empty()) {
        drawText(r, sub, statusMessage, 200, 660, SDL_Color{255, 160, 140, 255});
    }

    drawText(r, sub, "ESC: Close", 200, 700, UiTheme::Warm);
}
