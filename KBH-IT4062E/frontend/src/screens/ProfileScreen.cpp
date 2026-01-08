#include "ProfileScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"

void ProfileScreen::handleEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().change(RouteId::Title);
    } else if (e.key.keysym.sym == SDLK_c) {
        app->router().push(RouteId::ChangePasswordOverlay);
    } else if (e.key.keysym.sym == SDLK_o) {
        app->session().signOut();
        app->router().change(RouteId::Title);
    }
}

void ProfileScreen::render(SDL_Renderer* r) {
    SDL_SetRenderDrawColor(r, 18, 18, 26, 255);
    SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
    SDL_RenderFillRect(r, &dst);

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
    drawText(r, f, "Profile Screen (stub)", 80, 80, UiTheme::White);

    std::string u = app->session().isLoggedIn() ? app->session().user().username : "(not logged in)";
    drawText(r, f, "User: " + u, 80, 150, UiTheme::Warm);

    drawText(r, f, "Press C: Change password overlay", 80, 220, UiTheme::White);
    drawText(r, f, "Press O: Sign out", 80, 280, UiTheme::Yellow);
    drawText(r, f, "ESC: Back to Title", 80, 340, UiTheme::Yellow);
}
