#include "CreateAccountOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <SDL_image.h>
#include <iostream>

void CreateAccountOverlay::onEnter() {
    std::cout << "[CreateAccountOverlay] onEnter() called\n";
    username_.clear();
    password_.clear();
    confirmPassword_.clear();
    errorMessage_.clear();
    activeField_ = 0;
    std::cout << "[CreateAccountOverlay] Fields cleared\n";
    
    int frameX = (UiTheme::DesignW - 1163) / 2;
    int frameY = (UiTheme::DesignH - 652) / 2;
    std::cout << "[CreateAccountOverlay] Frame position: " << frameX << ", " << frameY << "\n";
    
    // Username field (offset from frame)
    usernameField_ = {frameX + 437, frameY + 178, 607, 64};
    
    // Password field
    passwordField_ = {frameX + 437, frameY + 296, 607, 64};
    
    // Confirm password field
    confirmPasswordField_ = {frameX + 437, frameY + 399, 607, 64};
    
    // Sign up button
    signUpButton_ = {frameX + 899, frameY + 493, 145, 51};
    
    // Exit button
    exitButton_ = {frameX + 1023, frameY + 26, 85, 85};
    std::cout << "[CreateAccountOverlay] Rects initialized\n";
    
    std::cout << "[CreateAccountOverlay] Loading exit button texture from: " << UiTheme::ExitButtonPath << "\n";
    exitButtonTexture_ = IMG_LoadTexture(app->renderer(), UiTheme::ExitButtonPath);
    if (!exitButtonTexture_) {
        std::cerr << "[CreateAccountOverlay] Failed to load exit button texture: " << IMG_GetError() << "\n";
    } else {
        std::cout << "[CreateAccountOverlay] Exit button texture loaded\n";
    }
    
    std::cout << "[CreateAccountOverlay] Starting text input\n";
    SDL_StartTextInput();
    std::cout << "[CreateAccountOverlay] onEnter() complete\n";
}

void CreateAccountOverlay::onExit() {
    SDL_StopTextInput();
    if (exitButtonTexture_) {
        SDL_DestroyTexture(exitButtonTexture_);
        exitButtonTexture_ = nullptr;
    }
}

void CreateAccountOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        
        if (isPointInRect(mx, my, usernameField_)) {
            activeField_ = 0;
            return;
        }
        if (isPointInRect(mx, my, passwordField_)) {
            activeField_ = 1;
            return;
        }
        if (isPointInRect(mx, my, confirmPasswordField_)) {
            activeField_ = 2;
            return;
        }
        if (isPointInRect(mx, my, exitButton_)) {
            handleExit();
            return;
        }
        if (isPointInRect(mx, my, signUpButton_)) {
            handleSignUp();
            return;
        }
    }
    
    if (e.type == SDL_TEXTINPUT) {
        std::string* activeFieldPtr = nullptr;
        if (activeField_ == 0) activeFieldPtr = &username_;
        else if (activeField_ == 1) activeFieldPtr = &password_;
        else if (activeField_ == 2) activeFieldPtr = &confirmPassword_;
        
        if (activeFieldPtr) {
            *activeFieldPtr += e.text.text;
        }
        return;
    }

    if (e.type != SDL_KEYDOWN) return;

    if (e.key.keysym.sym == SDLK_ESCAPE) {
        handleExit();
        return;
    }

    if (e.key.keysym.sym == SDLK_BACKSPACE) {
        std::string* activeFieldPtr = nullptr;
        if (activeField_ == 0) activeFieldPtr = &username_;
        else if (activeField_ == 1) activeFieldPtr = &password_;
        else if (activeField_ == 2) activeFieldPtr = &confirmPassword_;
        
        if (activeFieldPtr && !activeFieldPtr->empty()) {
            activeFieldPtr->pop_back();
        }
        return;
    }
    
    if (e.key.keysym.sym == SDLK_TAB) {
        activeField_ = (activeField_ + 1) % 3;
        return;
    }

    if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
        handleSignUp();
        return;
    }
}

void CreateAccountOverlay::render(SDL_Renderer* r) {
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
    
    // Title "Sign Up"
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 68);
    if (titleFont) {
        drawText(r, titleFont, "Sign Up", frameX + 76, frameY + 25, UiTheme::White);
    }
    
    // Divider line
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect divider{frameX + 75, frameY + 117, 1014, 3};
    SDL_RenderFillRect(r, &divider);
    
    // Labels
    TTF_Font* labelFont = app->resources().font(UiTheme::SubFontPath, 40);
    TTF_Font* inputFont = app->resources().font(UiTheme::SubFontPath, 36);
    
    if (labelFont) {
        drawText(r, labelFont, "Username:", frameX + 219, frameY + 174, UiTheme::White);
        drawText(r, labelFont, "Password:", frameX + 224, frameY + 292, UiTheme::White);
        drawText(r, labelFont, "Confirm password:", frameX + 105, frameY + 416, UiTheme::White);
    }
    
    // Username field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &usernameField_);
    if (activeField_ == 0) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &usernameField_);
    }
    if (inputFont && !username_.empty()) {
        drawText(r, inputFont, username_, usernameField_.x + 15, usernameField_.y + 14, UiTheme::White);
    }
    
    // Password field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &passwordField_);
    if (activeField_ == 1) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &passwordField_);
    }
    if (inputFont && !password_.empty()) {
        std::string masked(password_.length(), '*');
        drawText(r, inputFont, masked, passwordField_.x + 15, passwordField_.y + 14, UiTheme::White);
    }
    
    // Confirm password field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &confirmPasswordField_);
    if (activeField_ == 2) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &confirmPasswordField_);
    }
    if (inputFont && !confirmPassword_.empty()) {
        std::string masked(confirmPassword_.length(), '*');
        drawText(r, inputFont, masked, confirmPasswordField_.x + 15, confirmPasswordField_.y + 14, UiTheme::White);
    }
    
    // Error message (font size 26 = 28 - 2)
    TTF_Font* errorFont = app->resources().font(UiTheme::SubFontPath, 26);
    if (!errorMessage_.empty() && errorFont) {
        drawText(r, errorFont, errorMessage_, frameX + 457, frameY + 493, {252, 20, 20, 255});
    }
    
    // Sign up button
    SDL_SetRenderDrawColor(r, 22, 28, 86, 255);
    SDL_RenderFillRect(r, &signUpButton_);
    TTF_Font* buttonFont = app->resources().font(UiTheme::SubFontPath, 36);
    if (buttonFont) {
        drawText(r, buttonFont, "Sign up", signUpButton_.x + 15, signUpButton_.y + 8, UiTheme::White);
    }
    
    // Exit button
    if (exitButtonTexture_) {
        SDL_RenderCopy(r, exitButtonTexture_, nullptr, &exitButton_);
    }
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void CreateAccountOverlay::handleSignUp() {
    if (username_.empty()) {
        errorMessage_ = "Username cannot be empty";
        return;
    }
    
    if (password_.empty()) {
        errorMessage_ = "Password cannot be empty";
        return;
    }
    
    if (confirmPassword_.empty()) {
        errorMessage_ = "Please confirm your password";
        return;
    }
    
    if (password_ != confirmPassword_) {
        errorMessage_ = "Passwords do not match";
        return;
    }
    
    // Send create_account request to backend
    app->network().send_create_account(username_, password_);
    errorMessage_ = "Creating account...";
}

void CreateAccountOverlay::handleExit() {
    app->router().pop();
}

bool CreateAccountOverlay::isPointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && x < rect.x + rect.w &&
           y >= rect.y && y < rect.y + rect.h;
}
