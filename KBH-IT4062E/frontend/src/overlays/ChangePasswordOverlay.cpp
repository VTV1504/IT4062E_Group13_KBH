#include "ChangePasswordOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <jsoncpp/json/json.h>

void ChangePasswordOverlay::onEnter() {
    oldPassword.clear();
    newPassword.clear();
    confirm.clear();
    statusMessage.clear();
    activeField = 0;
    awaitingResponse = false;
    SDL_StartTextInput();
}

void ChangePasswordOverlay::onExit() {
    SDL_StopTextInput();
}

void ChangePasswordOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT) {
        if (activeField == 0) oldPassword += e.text.text;
        else if (activeField == 1) newPassword += e.text.text;
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
        if (activeField == 0 && !oldPassword.empty()) oldPassword.pop_back();
        if (activeField == 1 && !newPassword.empty()) newPassword.pop_back();
        if (activeField == 2 && !confirm.empty()) confirm.pop_back();
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        if (newPassword != confirm) {
            statusMessage = "Passwords do not match";
            return;
        }

        Json::Value payload;
        payload["old_password"] = oldPassword;
        payload["new_password"] = newPassword;
        app->network().sendCommand("CHANGE_PASSWORD", payload);
        awaitingResponse = true;
        statusMessage = "Updating password...";
    }
}

void ChangePasswordOverlay::update(float) {
    if (!awaitingResponse) return;

    AuthResult result;
    if (app->state().takeAuthResult(result)) {
        awaitingResponse = false;
        if (result.success) {
            statusMessage = "Password updated";
            app->router().pop();
        } else {
            statusMessage = result.message.empty() ? "Update failed" : result.message;
        }
    }
}

void ChangePasswordOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 200);

    SDL_SetRenderDrawColor(r, 40, 40, 55, 255);
    SDL_Rect panel{300, 170, 766, 480};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Change Password", 340, 210, UiTheme::White);

    SDL_Color active = UiTheme::Yellow;
    SDL_Color inactive = UiTheme::Warm;

    std::string maskedOld(oldPassword.size(), '*');
    drawText(r, f, "Old: " + (oldPassword.empty() ? std::string("_") : maskedOld), 340, 290,
             activeField == 0 ? active : inactive);
    std::string maskedNew(newPassword.size(), '*');
    drawText(r, f, "New: " + (newPassword.empty() ? std::string("_") : maskedNew), 340, 350,
             activeField == 1 ? active : inactive);
    std::string maskedConfirm(confirm.size(), '*');
    drawText(r, f, "Confirm: " + (confirm.empty() ? std::string("_") : maskedConfirm), 340, 410,
             activeField == 2 ? active : inactive);

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    drawText(r, sub, "ENTER: Update | TAB: switch", 340, 480, UiTheme::White);
    drawText(r, sub, "ESC: Close", 340, 520, UiTheme::Yellow);

    if (!statusMessage.empty()) {
        drawText(r, sub, statusMessage, 340, 560, UiTheme::Warm);
    }
}
