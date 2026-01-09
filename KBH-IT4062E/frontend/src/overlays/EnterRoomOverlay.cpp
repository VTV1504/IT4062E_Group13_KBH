#include "EnterRoomOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <jsoncpp/json/json.h>

void EnterRoomOverlay::onEnter() {
    code.clear();
    statusMessage.clear();
    mode = "arena";
    matchmaking = false;
    awaitingResponse = false;
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

    if (e.key.keysym.sym == SDLK_1) {
        mode = "arena";
        return;
    }

    if (e.key.keysym.sym == SDLK_2) {
        mode = "survival";
        return;
    }

    if (e.key.keysym.sym == SDLK_m) {
        matchmaking = !matchmaking;
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        Json::Value payload;
        payload["mode"] = mode;
        payload["matchmaking"] = matchmaking;
        if (!matchmaking) {
            payload["room_code"] = code;
        }
        app->network().sendCommand("JOIN_ROOM", payload);
        awaitingResponse = true;
        statusMessage = "Joining room...";
    }
}

void EnterRoomOverlay::update(float) {
    if (!awaitingResponse) return;

    RoomResult result;
    if (app->state().takeRoomResult(result)) {
        awaitingResponse = false;
        if (!result.success) {
            statusMessage = result.message.empty() ? "Join failed" : result.message;
        } else {
            app->router().pop();
        }
    }
}

void EnterRoomOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 190);

    SDL_SetRenderDrawColor(r, 35, 35, 48, 255);
    SDL_Rect panel{260, 170, 846, 480};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Join Room", 300, 210, UiTheme::White);

    drawText(r, f, "Code: " + (code.empty() ? std::string("_") : code), 300, 290, UiTheme::Warm);

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    drawText(r, sub, "Mode: " + mode + " (1: Arena, 2: Survival)", 300, 360, UiTheme::White);
    drawText(r, sub, std::string("Matchmaking: ") + (matchmaking ? "ON" : "OFF") + " (M)", 300, 400, UiTheme::White);
    drawText(r, sub, "ENTER: Join", 300, 440, UiTheme::Yellow);
    drawText(r, sub, "ESC: Close", 300, 480, UiTheme::White);

    if (!statusMessage.empty()) {
        drawText(r, sub, statusMessage, 300, 520, UiTheme::Warm);
    }
}
