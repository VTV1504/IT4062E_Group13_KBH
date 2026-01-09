#include "LobbyScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <jsoncpp/json/json.h>

void LobbyScreen::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        Json::Value payload;
        app->network().sendCommand("LEAVE_ROOM", payload);
        app->router().change(RouteId::Title);
        return;
    }

    if (e.key.keysym.sym == SDLK_r) {
        bool ready = !app->state().isReady();
        app->state().setReady(ready);
        app->network().sendCommand(ready ? "READY" : "UNREADY", Json::Value(Json::objectValue));
        return;
    }

    if (e.key.keysym.sym == SDLK_s) {
        Json::Value payload;
        if (app->state().roomMode() == "arena") {
            payload["difficulty"] = selectedDifficulty;
        }
        app->network().sendCommand("START_GAME", payload);
        return;
    }

    if (e.key.keysym.sym == SDLK_d && app->state().roomMode() == "arena") {
        if (selectedDifficulty == "easy") selectedDifficulty = "medium";
        else if (selectedDifficulty == "medium") selectedDifficulty = "hard";
        else selectedDifficulty = "easy";
    }
}

void LobbyScreen::render(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 20, 25, 35, 255);
    SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
    SDL_RenderFillRect(r, &dst);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Lobby", 80, 70, UiTheme::White);

    std::string roomCode = app->state().roomCode().empty() ? "(pending)" : app->state().roomCode();
    drawText(r, f, "Room: " + roomCode, 80, 150, UiTheme::Warm);
    drawText(r, f, "Mode: " + (app->state().roomMode().empty() ? "-" : app->state().roomMode()), 80, 210, UiTheme::Warm);

    std::string role = app->state().isRoomHost() ? "Host" : "Player";
    drawText(r, f, "Role: " + role, 80, 270, UiTheme::Warm);
    drawText(r, f, std::string("Ready: ") + (app->state().isReady() ? "Yes" : "No"), 80, 330, UiTheme::White);
    if (app->state().allReady()) {
        drawText(r, f, "All players ready", 80, 360, UiTheme::Yellow);
    }

    if (app->state().roomMode() == "arena") {
        drawText(r, f, "Difficulty: " + selectedDifficulty, 80, 420, UiTheme::Yellow);
        drawText(r, f, "Press D to change difficulty", 80, 480, UiTheme::White);
    }

    if (app->state().isRoomHost()) {
        drawText(r, f, "Press S to start (host)", 80, 550, UiTheme::Yellow);
    }
    drawText(r, f, "Press R to toggle ready", 80, 610, UiTheme::White);
    drawText(r, f, "ESC: Leave room", 80, 670, UiTheme::Yellow);
}
