#include "CreateAccountOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <jsoncpp/json/json.h>

void CreateAccountOverlay::onEnter() {
    username.clear();
    password.clear();
    confirm.clear();
    statusMessage.clear();
    activeField = 0;
    awaitingResponse = false;
    SDL_StartTextInput();
}

void CreateAccountOverlay::onExit() {
    SDL_StopTextInput();
}

void CreateAccountOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT) {
        if (activeField == 0) username += e.text.text;
        else if (activeField == 1) password += e.text.text;
        else confirm += e.text.text;
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
        return;
    }

    if (e.key.keysym.sym == SDLK_TAB) {
        activeField = (activeField + 1) % 3;
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        if (activeField == 0 && !username.empty()) username.pop_back();
        if (activeField == 1 && !password.empty()) password.pop_back();
        if (activeField == 2 && !confirm.empty()) confirm.pop_back();
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        if (password != confirm) {
            statusMessage = "Passwords do not match";
            return;
        }

        Json::Value payload;
        payload["username"] = username;
        payload["password"] = password;
        app->network().sendCommand("REGISTER", payload);
        awaitingResponse = true;
        statusMessage = "Registering...";
    }
}

void CreateAccountOverlay::update(float) {
    if (!awaitingResponse) return;

    AuthResult result;
    if (app->state().takeAuthResult(result)) {
        awaitingResponse = false;
        if (result.success) {
            statusMessage = "Account created. Please sign in.";
            app->router().pop();
        } else {
            statusMessage = result.message.empty() ? "Registration failed" : result.message;
        }
    }
}

void CreateAccountOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 200);

    SDL_SetRenderDrawColor(r, 40, 40, 55, 255);
    SDL_Rect panel{300, 170, 766, 480};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Create Account", 340, 210, UiTheme::White);

    SDL_Color active = UiTheme::Yellow;
    SDL_Color inactive = UiTheme::Warm;

    drawText(r, f, "Username: " + (username.empty() ? std::string("_") : username), 340, 290,
             activeField == 0 ? active : inactive);
    std::string masked(password.size(), '*');
    drawText(r, f, "Password: " + (password.empty() ? std::string("_") : masked), 340, 350,
             activeField == 1 ? active : inactive);
    std::string maskedConfirm(confirm.size(), '*');
    drawText(r, f, "Confirm:  " + (confirm.empty() ? std::string("_") : maskedConfirm), 340, 410,
             activeField == 2 ? active : inactive);

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    drawText(r, sub, "ENTER: Register | TAB: switch", 340, 480, UiTheme::White);
    drawText(r, sub, "ESC: Close", 340, 520, UiTheme::Yellow);

    if (!statusMessage.empty()) {
        drawText(r, sub, statusMessage, 340, 560, UiTheme::Warm);
    }
}
