#include "JoinRoomOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <iostream>
#include <cctype>
#include <cmath>

static bool pointInRect(int x, int y, const SDL_Rect& r) {
    return (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
}

void JoinRoomOverlay::onEnter() {
    room_code.clear();
    error_msg.clear();
    show_error = false;
    hoveredBtn = -1;
    
    // Setup hit rects
    backLinkRect = {117, 933, 200, 40};
    joinBtnRect = {555, 446, 426, 90};
    randomBtnRect = {555, 616, 426, 90};
    inputRect = {684, 238, 650, 116};
    
    SDL_StartTextInput();
}

void JoinRoomOverlay::onExit() {
    room_code.clear();
    clear_error();
    SDL_StopTextInput();
}

void JoinRoomOverlay::onResume() {
    // Restart text input when returning from another overlay
    SDL_StartTextInput();
}

void JoinRoomOverlay::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(app->renderer(), &outW, &outH);
    if (outW <= 0) outW = UiTheme::DesignW;
    if (outH <= 0) outH = UiTheme::DesignH;
    
    lx = wx * UiTheme::DesignW / outW;
    ly = wy * UiTheme::DesignH / outH;
}

void JoinRoomOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        int mx, my;
        windowToLogical(e.motion.x, e.motion.y, mx, my);
        
        hoveredBtn = -1;
        if (pointInRect(mx, my, backLinkRect)) hoveredBtn = 0;
        else if (pointInRect(mx, my, joinBtnRect) && isJoinEnabled()) hoveredBtn = 1;
        else if (pointInRect(mx, my, randomBtnRect)) hoveredBtn = 2;
        return;
    }
    
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx, my;
        windowToLogical(e.button.x, e.button.y, mx, my);
        
        if (pointInRect(mx, my, backLinkRect)) {
            btn_back_to_menu_pressed();
        } else if (pointInRect(mx, my, joinBtnRect) && isJoinEnabled()) {
            btn_join_pressed();
        } else if (pointInRect(mx, my, randomBtnRect)) {
            btn_random_room_pressed();
        }
        return;
    }
    
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
            btn_back_to_menu_pressed();
            return;
        }
        
        if (e.key.keysym.sym == SDLK_RETURN && isJoinEnabled()) {
            btn_join_pressed();
            return;
        }
        
        if (e.key.keysym.sym == SDLK_BACKSPACE && !room_code.empty()) {
            room_code.pop_back();
            clear_error();
            return;
        }
    }
    
    if (e.type == SDL_TEXTINPUT) {
        if (room_code.length() < 6) {
            char ch = e.text.text[0];
            if (std::isalnum(ch)) {
                room_code += std::toupper(ch);
                clear_error();
            }
        }
    }
}

void JoinRoomOverlay::update(float) {
    // Nothing to update per frame
}

void JoinRoomOverlay::render(SDL_Renderer* r) {
    // Dim background
    drawOverlayDim(r, 200);
    
    // Main panel background
    SDL_SetRenderDrawColor(r, 42, 45, 74, 255);
    SDL_Rect panel{0, 0, 1536, 1024};
    SDL_RenderFillRect(r, &panel);
    
    // Top separator
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect topSep{65, 161, 1405, 3};
    SDL_RenderFillRect(r, &topSep);
    
    // Bottom separator
    SDL_Rect bottomSep{65, 875, 1405, 3};
    SDL_RenderFillRect(r, &bottomSep);
    
    // Title "Join room"
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 72);
    if (titleFont) {
        drawText(r, titleFont, "Join room", 63, 76, UiTheme::White);
        
        // Title underline
        SDL_Rect titleLine{74, 121, 249, 3};
        SDL_RenderFillRect(r, &titleLine);
    }
    
    // Label "Room code:"
    TTF_Font* labelFont = app->resources().font(UiTheme::MainFontPath, 56);
    if (labelFont) {
        drawText(r, labelFont, "Room code:", 360, 275, UiTheme::White);
    }
    
    // Input box
    drawRoundedRect(r, inputRect.x, inputRect.y, inputRect.w, inputRect.h, 12, 
                    {18, 22, 36, 255});
    
    // Input text
    TTF_Font* inputFont = app->resources().font(UiTheme::MainFontPath, 64);
    if (inputFont && !room_code.empty()) {
        drawText(r, inputFont, room_code, 744, 270, UiTheme::White);
    }
    
    // Error message (giữa input box và JOIN button)
    if (show_error && !error_msg.empty()) {
        TTF_Font* errorFont = app->resources().font(UiTheme::MainFontPath, 24);
        if (errorFont) {
            SDL_Color errorColor{255, 69, 58, 255};
            int textW, textH;
            TTF_SizeText(errorFont, error_msg.c_str(), &textW, &textH);
            int errorX = 768 - textW / 2; // Căn giữa màn hình theo chiều ngang
            int errorY = (354 + 446) / 2 - textH / 2; // Giữa input box và JOIN button
            drawText(r, errorFont, error_msg, errorX, errorY, errorColor);
        }
    }
    
    // JOIN button
    SDL_Color joinColor = isJoinEnabled() ? 
        (hoveredBtn == 1 ? SDL_Color{69, 72, 97, 255} : SDL_Color{49, 52, 77, 255}) :
        SDL_Color{49, 52, 77, 128};
    drawRoundedRect(r, joinBtnRect.x, joinBtnRect.y, joinBtnRect.w, joinBtnRect.h, 10, joinColor);
    
    TTF_Font* btnFont = app->resources().font(UiTheme::MainFontPath, 64);
    if (btnFont) {
        SDL_Color textColor = isJoinEnabled() ? UiTheme::White : SDL_Color{255, 255, 255, 128};
        int textW, textH;
        TTF_SizeText(btnFont, "JOIN", &textW, &textH);
        int textX = joinBtnRect.x + (joinBtnRect.w - textW) / 2;
        int textY = joinBtnRect.y + (joinBtnRect.h - textH) / 2;
        drawText(r, btnFont, "JOIN", textX, textY, textColor);
    }
    
    // "Or" separator
    SDL_Rect leftLine{577, 581, 143, 3};
    SDL_Rect rightLine{810, 581, 143, 3};
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_RenderFillRect(r, &leftLine);
    SDL_RenderFillRect(r, &rightLine);
    
    TTF_Font* orFont = app->resources().font(UiTheme::MainFontPath, 32);
    if (orFont) {
        drawText(r, orFont, "Or", 745, 570, UiTheme::White);
    }
    
    // Random room button
    SDL_Color randomColor = hoveredBtn == 2 ? 
        SDL_Color{69, 72, 97, 255} : SDL_Color{49, 52, 77, 255};
    drawRoundedRect(r, randomBtnRect.x, randomBtnRect.y, randomBtnRect.w, randomBtnRect.h, 10, randomColor);
    
    if (btnFont) {
        int textW, textH;
        TTF_SizeText(btnFont, "Random room", &textW, &textH);
        int textX = randomBtnRect.x + (randomBtnRect.w - textW) / 2;
        int textY = randomBtnRect.y + (randomBtnRect.h - textH) / 2;
        drawText(r, btnFont, "Random room", textX, textY, UiTheme::White);
    }
    
    // Back to menu link
    TTF_Font* linkFont = app->resources().font(UiTheme::MainFontPath, 36);
    if (linkFont) {
        SDL_Color linkColor = hoveredBtn == 0 ? UiTheme::Warm : UiTheme::White;
        drawText(r, linkFont, "Back to menu", backLinkRect.x, backLinkRect.y, linkColor);
        
        // Underline
        SDL_SetRenderDrawColor(r, linkColor.r, linkColor.g, linkColor.b, linkColor.a);
        SDL_Rect linkLine{backLinkRect.x, backLinkRect.y + 40, 200, 2};
        SDL_RenderFillRect(r, &linkLine);
    }
}

void JoinRoomOverlay::drawRoundedRect(SDL_Renderer* r, int x, int y, int w, int h, 
                                       int radius, SDL_Color fill) {
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
    
    // Main rect (center)
    SDL_Rect center{x + radius, y, w - 2*radius, h};
    SDL_RenderFillRect(r, &center);
    
    SDL_Rect left{x, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(r, &left);
    
    SDL_Rect right{x + w - radius, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(r, &right);
    
    // Simple corners (circles approximated by filled rects - good enough)
    for (int cx = 0; cx < radius; ++cx) {
        for (int cy = 0; cy < radius; ++cy) {
            if (cx*cx + cy*cy <= radius*radius) {
                // Top-left
                SDL_RenderDrawPoint(r, x + radius - cx, y + radius - cy);
                // Top-right
                SDL_RenderDrawPoint(r, x + w - radius + cx, y + radius - cy);
                // Bottom-left
                SDL_RenderDrawPoint(r, x + radius - cx, y + h - radius + cy);
                // Bottom-right
                SDL_RenderDrawPoint(r, x + w - radius + cx, y + h - radius + cy);
            }
        }
    }
}

bool JoinRoomOverlay::isJoinEnabled() const {
    return room_code.length() >= 4; // Minimum 4 chars, adjust as needed
}

void JoinRoomOverlay::set_join_result(bool ok, const std::string& msg) {
    if (ok) {
        clear_error();
        // Success - let external code handle navigation to LobbyScreen
        std::cout << "[JoinRoomOverlay] Join successful\n";
    } else {
        set_error(msg);
    }
}

void JoinRoomOverlay::set_error(const std::string& msg) {
    error_msg = msg;
    show_error = true;
}

void JoinRoomOverlay::clear_error() {
    error_msg.clear();
    show_error = false;
}

// Placeholder callbacks
void JoinRoomOverlay::btn_back_to_menu_pressed() {
    std::cout << "[JoinRoomOverlay] Back to menu pressed\n";
    app->router().pop();
}

void JoinRoomOverlay::btn_join_pressed() {
    std::cout << "[JoinRoomOverlay] Join pressed with code: " << room_code << "\n";
    // Send join room request
    app->network().send_join_room(room_code);
    // Wait for server response via set_join_result()
}

void JoinRoomOverlay::btn_random_room_pressed() {
    std::cout << "[JoinRoomOverlay] Random room pressed\n";
    // Send join random request
    app->network().send_join_random();
    // Wait for server response
}
