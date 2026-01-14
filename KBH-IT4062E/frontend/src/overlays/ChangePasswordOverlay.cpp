#include "ChangePasswordOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <SDL_image.h>
#include <iostream>

void ChangePasswordOverlay::onEnter() {
    std::cout << "[ChangePasswordOverlay] onEnter() called\n";
    std::cout << "[ChangePasswordOverlay] this = " << this << "\n";
    std::cout << "[ChangePasswordOverlay] app = " << app << "\n";
    
    std::cout << "[ChangePasswordOverlay] Clearing fields...\n";
    currentPassword_ = "";
    newPassword_ = "";
    confirmPassword_ = "";
    errorMessage_ = "";
    activeField_ = 0;
    std::cout << "[ChangePasswordOverlay] Fields cleared\n";
    
    std::cout << "[ChangePasswordOverlay] Calculating frame position...\n";
    int frameX = (UiTheme::DesignW - 1163) / 2;
    int frameY = (UiTheme::DesignH - 652) / 2;
    std::cout << "[ChangePasswordOverlay] Frame: " << frameX << ", " << frameY << "\n";
    
    std::cout << "[ChangePasswordOverlay] Setting rects...\n";
    // Current password field (same position as username in CreateAccount)
    currentPasswordField_ = {frameX + 437, frameY + 178, 607, 64};
    
    // New password field
    newPasswordField_ = {frameX + 437, frameY + 296, 607, 64};
    
    // Confirm password field
    confirmPasswordField_ = {frameX + 437, frameY + 399, 607, 64};
    
    // Change button
    changeButton_ = {frameX + 899, frameY + 493, 145, 51};
    
    // Exit button
    exitButton_ = {frameX + 1023, frameY + 26, 85, 85};
    std::cout << "[ChangePasswordOverlay] Rects set\n";
    
    std::cout << "[ChangePasswordOverlay] Loading exit button texture...\n";
    exitButtonTexture_ = IMG_LoadTexture(app->renderer(), UiTheme::ExitButtonPath);
    if (!exitButtonTexture_) {
        std::cerr << "[ChangePasswordOverlay] Failed to load exit button texture: " << IMG_GetError() << "\n";
    } else {
        std::cout << "[ChangePasswordOverlay] Exit button texture loaded\n";
    }
    
    std::cout << "[ChangePasswordOverlay] Starting text input...\n";
    SDL_StartTextInput();
    std::cout << "[ChangePasswordOverlay] onEnter() complete\n";
}

void ChangePasswordOverlay::onExit() {
    SDL_StopTextInput();
    if (exitButtonTexture_) {
        SDL_DestroyTexture(exitButtonTexture_);
        exitButtonTexture_ = nullptr;
    }
}

void ChangePasswordOverlay::onResume() {
    SDL_StartTextInput();
}

void ChangePasswordOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        
        if (isPointInRect(mx, my, currentPasswordField_)) {
            activeField_ = 0;
            return;
        }
        if (isPointInRect(mx, my, newPasswordField_)) {
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
        if (isPointInRect(mx, my, changeButton_)) {
            handleChange();
            return;
        }
    }
    
    if (e.type == SDL_TEXTINPUT) {
        std::string* activeFieldPtr = nullptr;
        if (activeField_ == 0) activeFieldPtr = &currentPassword_;
        else if (activeField_ == 1) activeFieldPtr = &newPassword_;
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
        if (activeField_ == 0) activeFieldPtr = &currentPassword_;
        else if (activeField_ == 1) activeFieldPtr = &newPassword_;
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
        handleChange();
        return;
    }
}

void ChangePasswordOverlay::render(SDL_Renderer* r) {
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
    
    // Title "Change Password"
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 68);
    if (titleFont) {
        drawText(r, titleFont, "Change Password", frameX + 76, frameY + 25, UiTheme::White);
    }
    
    // Divider line
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect divider{frameX + 75, frameY + 117, 1014, 3};
    SDL_RenderFillRect(r, &divider);
    
    // Labels
    TTF_Font* labelFont = app->resources().font(UiTheme::SubFontPath, 40);
    TTF_Font* inputFont = app->resources().font(UiTheme::SubFontPath, 36);
    
    if (labelFont) {
        drawText(r, labelFont, "Current password:", frameX + 115, frameY + 174, UiTheme::White);
        drawText(r, labelFont, "New password:", frameX + 169, frameY + 292, UiTheme::White);
        drawText(r, labelFont, "Confirm password:", frameX + 105, frameY + 416, UiTheme::White);
    }
    
    // Current password field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &currentPasswordField_);
    if (activeField_ == 0) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &currentPasswordField_);
    }
    if (inputFont && !currentPassword_.empty()) {
        std::string masked(currentPassword_.length(), '*');
        drawText(r, inputFont, masked, currentPasswordField_.x + 15, currentPasswordField_.y + 14, UiTheme::White);
    }
    
    // New password field
    SDL_SetRenderDrawColor(r, 18, 22, 36, 255);
    SDL_RenderFillRect(r, &newPasswordField_);
    if (activeField_ == 1) {
        SDL_SetRenderDrawColor(r, 100, 150, 255, 255);
        SDL_RenderDrawRect(r, &newPasswordField_);
    }
    if (inputFont && !newPassword_.empty()) {
        std::string masked(newPassword_.length(), '*');
        drawText(r, inputFont, masked, newPasswordField_.x + 15, newPasswordField_.y + 14, UiTheme::White);
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
    
    // Error message
    TTF_Font* errorFont = app->resources().font(UiTheme::SubFontPath, 26);
    if (!errorMessage_.empty() && errorFont) {
        drawText(r, errorFont, errorMessage_, frameX + 457, frameY + 493, {252, 20, 20, 255});
    }
    
    // Change button
    SDL_SetRenderDrawColor(r, 22, 28, 86, 255);
    SDL_RenderFillRect(r, &changeButton_);
    TTF_Font* buttonFont = app->resources().font(UiTheme::SubFontPath, 36);
    if (buttonFont) {
        drawText(r, buttonFont, "Change", changeButton_.x + 15, changeButton_.y + 8, UiTheme::White);
    }
    
    // Exit button
    if (exitButtonTexture_) {
        SDL_RenderCopy(r, exitButtonTexture_, nullptr, &exitButton_);
    }
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void ChangePasswordOverlay::handleChange() {
    if (currentPassword_.empty()) {
        errorMessage_ = "Current password cannot be empty";
        return;
    }
    
    if (newPassword_.empty()) {
        errorMessage_ = "New password cannot be empty";
        return;
    }
    
    if (confirmPassword_.empty()) {
        errorMessage_ = "Please confirm your new password";
        return;
    }
    
    if (newPassword_ != confirmPassword_) {
        errorMessage_ = "Passwords do not match";
        return;
    }
    
    // Get current username from AppState
    std::string username = app->state().getUsername();
    if (username.empty()) {
        errorMessage_ = "Not logged in";
        return;
    }
    
    // Send change_password request to backend
    app->network().send_change_password(username, currentPassword_, newPassword_);
    errorMessage_ = "Changing password...";
}

void ChangePasswordOverlay::handleExit() {
    app->router().pop();
}

bool ChangePasswordOverlay::isPointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && x < rect.x + rect.w &&
           y >= rect.y && y < rect.y + rect.h;
}
