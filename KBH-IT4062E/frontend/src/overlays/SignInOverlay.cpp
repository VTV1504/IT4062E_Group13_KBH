#include "SignInOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include "GuestResultOverlay.h"
#include <jsoncpp/json/json.h>

void SignInOverlay::onEnter() {
    username.clear();
    password.clear();
    statusMessage.clear();
    activeField = 0;
    awaitingResponse = false;
    SDL_StartTextInput();
}

void SignInOverlay::onExit() {
    SDL_StopTextInput();
}

void SignInOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT) {
        if (activeField == 0) username += e.text.text;
        else password += e.text.text;
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
        return;
    }

    if (e.key.keysym.sym == SDLK_TAB) {
        activeField = 1 - activeField;
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        if (activeField == 0 && !username.empty()) username.pop_back();
        if (activeField == 1 && !password.empty()) password.pop_back();
        return;
    }

    if (e.key.keysym.sym == SDLK_c) {
        app->router().push(RouteId::CreateAccountOverlay);
        return;
    }

    if (e.key.keysym.sym == SDLK_g) {
        Json::Value payload;
        payload["nickname"] = username.empty() ? "Guest" : username;
        app->network().sendCommand("GUEST", payload);
        awaitingResponse = true;
        statusMessage = "Joining as guest...";
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        Json::Value payload;
        payload["username"] = username;
        payload["password"] = password;
        app->network().sendCommand("LOGIN", payload);
        awaitingResponse = true;
        statusMessage = "Signing in...";
    }
}

void SignInOverlay::update(float) {
    if (!awaitingResponse) return;

    AuthResult result;
    if (app->state().takeAuthResult(result)) {
        awaitingResponse = false;
        if (result.success) {
            statusMessage = "Welcome, " + app->session().user().username;
            app->router().pop();

            if (auto* top = app->views().top()) {
                if (dynamic_cast<GuestResultOverlay*>(top)) {
                    app->router().replaceTop(RouteId::UserResultOverlay);
                }
            }
        } else {
            statusMessage = result.message.empty() ? "Sign in failed" : result.message;
        }
    }
}

void SignInOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 190);

    SDL_SetRenderDrawColor(r, 35, 35, 48, 255);
    SDL_Rect panel{260, 170, 846, 520};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 40);
    drawText(r, f, "Sign In", 300, 210, UiTheme::White);

    SDL_Color active = UiTheme::Yellow;
    SDL_Color inactive = UiTheme::Warm;

    drawText(r, f, "Username: " + (username.empty() ? std::string("_") : username), 300, 290,
             activeField == 0 ? active : inactive);
    std::string masked(password.size(), '*');
    drawText(r, f, "Password: " + (password.empty() ? std::string("_") : masked), 300, 350,
             activeField == 1 ? active : inactive);

    TTF_Font* sub = app->resources().font(UiTheme::SubFontPath, 24);
    drawText(r, sub, "ENTER: Sign in | TAB: switch | G: Guest", 300, 430, UiTheme::White);
    drawText(r, sub, "Press C: Create account", 300, 470, UiTheme::White);
    drawText(r, sub, "ESC: Close", 300, 510, UiTheme::Warm);

    if (!statusMessage.empty()) {
        drawText(r, sub, statusMessage, 300, 550, UiTheme::Yellow);
    }
}
