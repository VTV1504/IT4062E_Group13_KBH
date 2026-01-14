#include "GuestResultOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <iostream>
#include <sstream>
#include <iomanip>

void GuestResultOverlay::onEnter() {
    std::cout << "[GuestResultOverlay] onEnter()\n";
    
    // Load player result from GameEnd
    if (app->state().hasGameEnd()) {
        const auto& rankings = app->state().getGameEnd().rankings;
        if (!rankings.empty()) {
            playerResult = rankings[0];  // Training mode has only 1 player
            std::cout << "[GuestResultOverlay] Loaded result: " << playerResult.display_name
                      << " WPM=" << playerResult.wpm << " ACC=" << playerResult.accuracy << "%"
                      << " word_idx=" << playerResult.word_idx << "\n";
        } else {
            std::cout << "[GuestResultOverlay] No rankings data!\n";
        }
    } else {
        std::cout << "[GuestResultOverlay] No GameEnd event!\n";
    }
    
    // Define button hit rects relative to content frame position
    int frameX = (UiTheme::DesignW - 1163) / 2;
    int frameY = (UiTheme::DesignH - 652) / 2;
    
    // Exit menu button: left bottom (88, 572 relative to frame)
    exitMenuBtn = {frameX + 88, frameY + 572, 250, 50};
    
    // Login to Save Result button: right bottom (761, 570 relative to frame)
    loginBtn = {frameX + 761, frameY + 570, 380, 50};
}

void GuestResultOverlay::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(app->renderer(), &outW, &outH);
    if (outW <= 0) outW = UiTheme::DesignW;
    if (outH <= 0) outH = UiTheme::DesignH;
    
    lx = wx * UiTheme::DesignW / outW;
    ly = wy * UiTheme::DesignH / outH;
}

void GuestResultOverlay::handleExitMenu() {
    std::cout << "[GuestResultOverlay] Exit menu clicked\n";
    app->router().change(RouteId::Title);
}

void GuestResultOverlay::handleLoginToSave() {
    std::cout << "[GuestResultOverlay] Login to Save clicked\n";
    app->router().push(RouteId::SignInOverlay);
}

void GuestResultOverlay::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        int wx = e.motion.x;
        int wy = e.motion.y;
        int lx, ly;
        windowToLogical(wx, wy, lx, ly);
        
        hoveredBtn = -1;
        if (lx >= exitMenuBtn.x && lx < exitMenuBtn.x + exitMenuBtn.w &&
            ly >= exitMenuBtn.y && ly < exitMenuBtn.y + exitMenuBtn.h) {
            hoveredBtn = 0;
        }
        else if (lx >= loginBtn.x && lx < loginBtn.x + loginBtn.w &&
                 ly >= loginBtn.y && ly < loginBtn.y + loginBtn.h) {
            hoveredBtn = 1;
        }
    }
    
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int wx = e.button.x;
        int wy = e.button.y;
        int lx, ly;
        windowToLogical(wx, wy, lx, ly);
        
        // Check exit menu button
        if (lx >= exitMenuBtn.x && lx < exitMenuBtn.x + exitMenuBtn.w &&
            ly >= exitMenuBtn.y && ly < exitMenuBtn.y + exitMenuBtn.h) {
            handleExitMenu();
        }
        // Check login button
        else if (lx >= loginBtn.x && lx < loginBtn.x + loginBtn.w &&
                 ly >= loginBtn.y && ly < loginBtn.y + loginBtn.h) {
            handleLoginToSave();
        }
    }
    
    // ESC to exit
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        handleExitMenu();
    }
}

void GuestResultOverlay::render(SDL_Renderer* r) {
    // Semi-transparent background overlay: rgba(30, 31, 52, 0.8)
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 30, 31, 52, (Uint8)(0.8 * 255));
    SDL_Rect fullScreen = {0, 0, UiTheme::DesignW, UiTheme::DesignH};
    SDL_RenderFillRect(r, &fullScreen);
    
    // Content frame: 1163x652 centered, bg color #2a2d4a
    SDL_SetRenderDrawColor(r, 0x2a, 0x2d, 0x4a, 255);
    SDL_Rect contentFrame = {
        (UiTheme::DesignW - 1163) / 2,
        (UiTheme::DesignH - 652) / 2,
        1163,
        652
    };
    SDL_RenderFillRect(r, &contentFrame);
    
    int frameX = contentFrame.x;
    int frameY = contentFrame.y;
    
    // Title: "Training result" at (77, 25) relative to frame, font size 68
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 68);
    if (titleFont) {
        drawText(r, titleFont, "Training result", frameX + 77, frameY + 25, UiTheme::White);
    }
    
    // Horizontal lines (3px solid #c2c2c2)
    SDL_SetRenderDrawColor(r, 0xc2, 0xc2, 0xc2, 255);
    
    // Line 1: top=116.5px, centered width=1014px
    for (int i = 0; i < 3; i++) {
        SDL_RenderDrawLine(r, 
            frameX + (1163 - 1014) / 2, 
            frameY + 116 + i,
            frameX + (1163 - 1014) / 2 + 1014, 
            frameY + 116 + i);
    }
    
    // Line 2: top=178.5px, centered width=702px
    for (int i = 0; i < 3; i++) {
        SDL_RenderDrawLine(r,
            frameX + (1163 - 702) / 2,
            frameY + 178 + i,
            frameX + (1163 - 702) / 2 + 702,
            frameY + 178 + i);
    }
    
    // Line 3: top=248.5px, centered width=702px
    for (int i = 0; i < 3; i++) {
        SDL_RenderDrawLine(r,
            frameX + (1163 - 702) / 2,
            frameY + 248 + i,
            frameX + (1163 - 702) / 2 + 702,
            frameY + 248 + i);
    }
    
    // Line 4: top=555.5px, centered width=1014px
    for (int i = 0; i < 3; i++) {
        SDL_RenderDrawLine(r,
            frameX + (1163 - 1014) / 2,
            frameY + 555 + i,
            frameX + (1163 - 1014) / 2 + 1014,
            frameY + 555 + i);
    }
    
    // Table headers (font size 32)
    TTF_Font* headerFont = app->resources().font(UiTheme::MainFontPath, 32);
    if (headerFont) {
        // No. at (232, 143)
        drawText(r, headerFont, "No.", frameX + 232, frameY + 143, UiTheme::White);
        
        // Player at (326, 146)
        drawText(r, headerFont, "Player", frameX + 326, frameY + 146, UiTheme::White);
        
        // WPM at (730, 143)
        drawText(r, headerFont, "WPM", frameX + 730, frameY + 143, UiTheme::White);
        
        // Acc. at (832, 142)
        drawText(r, headerFont, "Acc.", frameX + 832, frameY + 142, UiTheme::White);
    }
    
    // Result row background: linear-gradient(90deg, #633d21, rgba(140, 88, 42, 0.71))
    // at (239, 193) size 685x41
    SDL_Rect rowBg = {frameX + 239, frameY + 193, 685, 41};
    
    // Simple approximation: draw with middle color
    SDL_SetRenderDrawColor(r, 0x63 + (0x8c - 0x63) / 2, 0x3d + (0x58 - 0x3d) / 2, 0x21 + (0x2a - 0x21) / 2, 200);
    SDL_RenderFillRect(r, &rowBg);
    
    // Player data (font size 24)
    TTF_Font* dataFont = app->resources().font(UiTheme::MainFontPath, 24);
    if (dataFont) {
        // "1." at (251, 203)
        drawText(r, dataFont, "1.", frameX + 251, frameY + 203, UiTheme::White);
        
        // Player name at (326, 205)
        drawText(r, dataFont, playerResult.display_name, frameX + 326, frameY + 205, UiTheme::White);
        
        // WPM at (767, 205)
        std::ostringstream wpmStr;
        wpmStr << std::fixed << std::setprecision(0) << playerResult.wpm;
        drawText(r, dataFont, wpmStr.str(), frameX + 767, frameY + 205, UiTheme::White);
        
        // Accuracy at (861, 205)
        std::ostringstream accStr;
        accStr << std::fixed << std::setprecision(1) << playerResult.accuracy << "%";
        drawText(r, dataFont, accStr.str(), frameX + 861, frameY + 205, UiTheme::White);
    }
    
    // Bottom buttons (font size 42, underlined)
    TTF_Font* btnFont = app->resources().font(UiTheme::MainFontPath, 42);
    if (btnFont) {
        // Exit menu at (88, 572)
        SDL_Color exitColor = (hoveredBtn == 0) ? UiTheme::Yellow : UiTheme::White;
        drawText(r, btnFont, "Exit menu", frameX + 88, frameY + 572, exitColor);
        if (hoveredBtn == 0) {
            // Draw underline
            int textW = 0;
            TTF_SizeText(btnFont, "Exit menu", &textW, nullptr);
            SDL_SetRenderDrawColor(r, exitColor.r, exitColor.g, exitColor.b, exitColor.a);
            SDL_RenderDrawLine(r, frameX + 88, frameY + 572 + 50, frameX + 88 + textW, frameY + 572 + 50);
        }
        
        // Login to Save Result at (761, 570)
        SDL_Color loginColor = (hoveredBtn == 1) ? UiTheme::Yellow : UiTheme::White;
        drawText(r, btnFont, "Login to Save Result", frameX + 761, frameY + 570, loginColor);
        if (hoveredBtn == 1) {
            // Draw underline
            int textW = 0;
            TTF_SizeText(btnFont, "Login to Save Result", &textW, nullptr);
            SDL_SetRenderDrawColor(r, loginColor.r, loginColor.g, loginColor.b, loginColor.a);
            SDL_RenderDrawLine(r, frameX + 761, frameY + 570 + 50, frameX + 761 + textW, frameY + 570 + 50);
        }
    }
}
