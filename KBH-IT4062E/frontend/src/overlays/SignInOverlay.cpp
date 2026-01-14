#include "SignInOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <SDL_image.h>

void SignInOverlay::onEnter() {
    username_.clear();
    password_.clear();
    errorMessage_.clear();
    usernameActive_ = true;
    
    int frameX = (UiTheme::DesignW - 1163) / 2;
    int frameY = (UiTheme::DesignH - 652) / 2;
    
    // Username field bên phải label "Username:" (130 + 200 offset for label)
    usernameField_ = {frameX + 330, frameY + 168, 715, 70};
    
    // Password field bên phải label "Password:" (130 + 200 offset for label)
    passwordField_ = {frameX + 330, frameY + 268, 715, 70};
    
    // Sign in button bên phải error text area
    signInButton_ = {frameX + 900, frameY + 368, 134, 61};
    
    // Create account button ở dưới
    createAccountButton_ = {frameX + 429, frameY + 553, 305, 50};
    
    // Exit button ở góc trên phải
    exitButton_ = {frameX + 1023, frameY + 26, 85, 85};
    
    exitButtonTexture_ = IMG_LoadTexture(app->renderer(), UiTheme::ExitButtonPath);
    
    SDL_StartTextInput();
}

void SignInOverlay::onExit() {
    SDL_StopTextInput();
    if (exitButtonTexture_) {
        SDL_DestroyTexture(exitButtonTexture_);
        exitButtonTexture_ = nullptr;
    }
}

void SignInOverlay::onResume() {
    // Restart text input when returning from CreateAccountOverlay
    SDL_StartTextInput();
}

void SignInOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        
        if (isPointInRect(mx, my, usernameField_)) {
            usernameActive_ = true;
            return;
        }
        if (isPointInRect(mx, my, passwordField_)) {
            usernameActive_ = false;
            return;
        }
        if (isPointInRect(mx, my, exitButton_)) {
            handleExit();
            return;
        }
        if (isPointInRect(mx, my, signInButton_)) {
            handleSignIn();
            return;
        }
        if (isPointInRect(mx, my, createAccountButton_)) {
            handleCreateAccount();
            return;
        }
    }
    
    if (e.type == SDL_TEXTINPUT) {
        std::string& activeField = usernameActive_ ? username_ : password_;
        activeField += e.text.text;
        // Keep error message visible - don't clear on typing
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        handleExit();
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        std::string& activeField = usernameActive_ ? username_ : password_;
        if (!activeField.empty()) activeField.pop_back();
        // Keep error message visible - don't clear on backspace
        return;
    }
    
    if (e.key.keysym.sym == SDLK_TAB) {
        usernameActive_ = !usernameActive_;
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        handleSignIn();
        return;
    }
}

void SignInOverlay::render(SDL_Renderer* r) {
    // Semi-transparent overlay background
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 30, 31, 52, 204);
    SDL_Rect fullScreen{0, 0, UiTheme::DesignW, UiTheme::DesignH};
    SDL_RenderFillRect(r, &fullScreen);
    
    int frameX = (UiTheme::DesignW - 1163) / 2;
    int frameY = (UiTheme::DesignH - 652) / 2;
    SDL_SetRenderDrawColor(r, 42, 45, 74, 255);
    SDL_Rect formFrame{frameX, frameY, 1163, 652};
    SDL_RenderFillRect(r, &formFrame);
    
    // Title
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 68);
    if (titleFont) {
        drawText(r, titleFont, "Sign In", frameX + 79, frameY + 22, UiTheme::White);
    }
    
    // Divider
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect divider{frameX + 75, frameY + 117, 1014, 3};
    SDL_RenderFillRect(r, &divider);
    
    // Labels
    TTF_Font* labelFont = app->resources().font(UiTheme::SubFontPath, 40);
    TTF_Font* inputFont = app->resources().font(UiTheme::SubFontPath, 36);
    
    if (labelFont) {
        drawText(r, labelFont, "Username:", frameX + 130, frameY + 178, UiTheme::White);
        drawText(r, labelFont, "Password:", frameX + 130, frameY + 278, UiTheme::White);
    }
    
    // Username field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &usernameField_);
    if (usernameActive_) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &usernameField_);
    }
    if (inputFont && !username_.empty()) {
        drawText(r, inputFont, username_, usernameField_.x + 15, usernameField_.y + 17, UiTheme::White);
    }
    
    // Password field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &passwordField_);
    if (!usernameActive_) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &passwordField_);
    }
    if (inputFont && !password_.empty()) {
        std::string masked(password_.length(), '*');
        drawText(r, inputFont, masked, passwordField_.x + 15, passwordField_.y + 17, UiTheme::White);
    }
    
    // Error message (smaller font)
    TTF_Font* errorFont = app->resources().font(UiTheme::SubFontPath, 38);
    if (!errorMessage_.empty() && errorFont) {
        drawText(r, errorFont, errorMessage_, frameX + 130, frameY + 378, {252, 20, 20, 255});
    }
    
    // Sign in button
    SDL_SetRenderDrawColor(r, 22, 28, 86, 255);
    SDL_RenderFillRect(r, &signInButton_);
    if (labelFont) {
        drawText(r, labelFont, "GO", signInButton_.x + 45, signInButton_.y + 5, UiTheme::White);
    }
    
    // Or divider
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect orLine1{frameX + 384, frameY + 503, 155, 4};
    SDL_RenderFillRect(r, &orLine1);
    if (labelFont) {
        drawText(r, labelFont, "Or", frameX + 559, frameY + 481, UiTheme::White);
    }
    SDL_Rect orLine2{frameX + 624, frameY + 503, 155, 4};
    SDL_RenderFillRect(r, &orLine2);
    
    // Create account button
    if (labelFont) {
        drawText(r, labelFont, "Create new account", createAccountButton_.x, createAccountButton_.y, UiTheme::White);
    }
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_Rect underline{createAccountButton_.x, createAccountButton_.y + 45, 305, 2};
    SDL_RenderFillRect(r, &underline);
    
    // Exit button
    if (exitButtonTexture_) {
        SDL_RenderCopy(r, exitButtonTexture_, nullptr, &exitButton_);
    }
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void SignInOverlay::handleSignIn() {
    if (username_.empty()) {
        errorMessage_ = "Username cannot be empty";
        return;
    }
    
    if (password_.empty()) {
        errorMessage_ = "Password cannot be empty";
        return;
    }
    
    // Send sign_in request to backend
    app->network().send_sign_in(username_, password_);
    errorMessage_ = "Signing in...";
}

void SignInOverlay::handleCreateAccount() {
    app->router().push(RouteId::CreateAccountOverlay);
}

void SignInOverlay::handleExit() {
    app->router().pop();
}

bool SignInOverlay::isPointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && x < rect.x + rect.w &&
           y >= rect.y && y < rect.y + rect.h;
}
