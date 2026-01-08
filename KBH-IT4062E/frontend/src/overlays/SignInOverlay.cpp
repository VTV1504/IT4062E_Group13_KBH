#include "SignInOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include "GuestResultOverlay.h"

void SignInOverlay::onEnter() {
    username.clear();
    SDL_StartTextInput();
}

void SignInOverlay::onExit() {
    SDL_StopTextInput();
}

void SignInOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_TEXTINPUT) {
        username += e.text.text;
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        if (!username.empty()) username.pop_back();
        return;
    }

    if (e.key.keysym.sym == SDLK_c) {
        app->router().push(RouteId::CreateAccountOverlay);
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        std::string u = username.empty() ? "player" : username;
        app->session().signIn(u);

        app->router().pop(); // close SignIn

        // After popping, if top is GuestResultOverlay => upgrade to UserResultOverlay
        if (auto* top = app->views().top()) {
            if (dynamic_cast<GuestResultOverlay*>(top)) {
                app->router().replaceTop(RouteId::UserResultOverlay);
            }
        }
    }
}

void SignInOverlay::render(SDL_Renderer* r) {
    drawOverlayDim(r, 190);

    SDL_SetRenderDrawColor(r, 35, 35, 48, 255);
    SDL_Rect panel{260, 170, 846, 420};
    SDL_RenderFillRect(r, &panel);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Sign In (stub)", 300, 210, UiTheme::White);

    drawText(r, f, "Username: " + (username.empty() ? std::string("_") : username), 300, 290, UiTheme::Warm);
    drawText(r, f, "Type name, ENTER to sign in", 300, 370, UiTheme::White);
    drawText(r, f, "Press C: Create account overlay", 300, 440, UiTheme::Yellow);
    drawText(r, f, "ESC: Close", 300, 510, UiTheme::White);
}
