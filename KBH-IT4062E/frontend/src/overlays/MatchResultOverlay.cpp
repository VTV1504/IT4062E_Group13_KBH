#include "MatchResultOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void MatchResultOverlay::onEnter() {
    std::cout << "[MatchResultOverlay] onEnter()\n";
    
    // Load rankings from AppState
    if (app->state().hasGameEnd()) {
        rankings = app->state().getGameEnd().rankings;
        std::cout << "[MatchResultOverlay] Loaded " << rankings.size() << " rankings\n";
        for (const auto& r : rankings) {
            std::cout << "  Rank " << r.rank << ": " << r.display_name 
                      << " WPM=" << r.wpm << " ACC=" << r.accuracy << "%\n";
        }
    }
    
    // Define button hit rects relative to content frame position
    int frameX = (UiTheme::DesignW - 1163) / 2;
    int frameY = (UiTheme::DesignH - 652) / 2;
    
    // Exit menu button: left side, bottom (88, 572 relative to frame)
    exitMenuBtn = {frameX + 88, frameY + 572, 250, 50};
    
    // Return lobby button: right side, bottom (869, 572 relative to frame)
    returnLobbyBtn = {frameX + 869, frameY + 572, 280, 50};
}

void MatchResultOverlay::handleEvent(const SDL_Event& e) {
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
        else if (lx >= returnLobbyBtn.x && lx < returnLobbyBtn.x + returnLobbyBtn.w &&
                 ly >= returnLobbyBtn.y && ly < returnLobbyBtn.y + returnLobbyBtn.h) {
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
        // Check return lobby button
        else if (lx >= returnLobbyBtn.x && lx < returnLobbyBtn.x + returnLobbyBtn.w &&
                 ly >= returnLobbyBtn.y && ly < returnLobbyBtn.y + returnLobbyBtn.h) {
            handleReturnLobby();
        }
    }
    
    // ESC to close overlay (return to lobby)
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        handleReturnLobby();
    }
}

void MatchResultOverlay::render(SDL_Renderer* r) {
    // Semi-transparent background overlay: rgba(30, 31, 52, 0.8)
    drawOverlayDim(r, (Uint8)(0.8 * 255));
    
    // Content frame: 1163x652 centered, bg color #2a2d4a
    SDL_SetRenderDrawColor(r, 0x2a, 0x2d, 0x4a, 255);
    SDL_Rect contentFrame = {
        (UiTheme::DesignW - 1163) / 2,  // centered horizontally
        (UiTheme::DesignH - 652) / 2,   // centered vertically
        1163,
        652
    };
    SDL_RenderFillRect(r, &contentFrame);
    
    int frameX = contentFrame.x;
    int frameY = contentFrame.y;
    
    // Title: "Match result" at (77, 25) relative to frame, font size 68
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 68);
    if (titleFont) {
        drawText(r, titleFont, "Match result", frameX + 77, frameY + 25, UiTheme::White);
    }
    
    // Horizontal lines
    SDL_SetRenderDrawColor(r, 0xc2, 0xc2, 0xc2, 255);
    
    // Line 1: top: 116.5px, width: 1014px, centered horizontally
    SDL_Rect line1 = {frameX + (1163 - 1014) / 2, frameY + 116, 1014, 3};
    SDL_RenderFillRect(r, &line1);
    
    // Line 2: top: 178.5px, width: 702px, centered
    SDL_Rect line2 = {frameX + (1163 - 702) / 2, frameY + 178, 702, 3};
    SDL_RenderFillRect(r, &line2);
    
    // Line 3: top: 532.5px, width: 702px
    SDL_Rect line3 = {frameX + (1163 - 702) / 2, frameY + 532, 702, 3};
    SDL_RenderFillRect(r, &line3);
    
    // Line 4: top: 555.5px, width: 1014px
    SDL_Rect line4 = {frameX + (1163 - 1014) / 2, frameY + 555, 1014, 3};
    SDL_RenderFillRect(r, &line4);
    
    // Column headers: No., Player, WPM, Acc.
    TTF_Font* headerFont = app->resources().font(UiTheme::MainFontPath, 32);
    if (headerFont) {
        drawText(r, headerFont, "No.", frameX + 232, frameY + 143, UiTheme::White);
        drawText(r, headerFont, "Player", frameX + 326, frameY + 146, UiTheme::White);
        drawText(r, headerFont, "WPM", frameX + 730, frameY + 143, UiTheme::White);
        drawText(r, headerFont, "Acc.", frameX + 832, frameY + 142, UiTheme::White);
    }
    
    // Render ranking rows (up to 6)
    int rowYStart = frameY + 193;
    int rowHeight = 41;
    int rowSpacing = 0; // rows are back-to-back, but alternating colors
    
    for (int i = 0; i < (int)rankings.size() && i < 6; ++i) {
        int rowY = rowYStart + i * (rowHeight + rowSpacing);
        renderRankingRow(r, i, rankings[i], rowY, i == 0);
    }
    
    // Bottom buttons
    TTF_Font* buttonFont = app->resources().font(UiTheme::MainFontPath, 42);
    if (buttonFont) {
        SDL_Color exitColor = (hoveredBtn == 0) ? UiTheme::Yellow : UiTheme::White;
        SDL_Color returnColor = (hoveredBtn == 1) ? UiTheme::Yellow : UiTheme::White;
        
        drawText(r, buttonFont, "Exit menu", frameX + 88, frameY + 572, exitColor);
        drawText(r, buttonFont, "Return lobby", frameX + 869, frameY + 572, returnColor);
    }
}

void MatchResultOverlay::renderRankingRow(SDL_Renderer* r, int rowIndex, const RankingData& data, int y, bool isFirst) {
    (void)rowIndex; // Currently unused
    int frameX = (UiTheme::DesignW - 1163) / 2;
    
    // Row background: gradient from #633d21 to rgba(140, 88, 42, 0.71) for rank 1
    // or from #63482c to #6b5631 for others
    // Width: 685px, height: 41px
    // Position: x = 239 (relative to frame)
    
    SDL_Rect rowRect = {frameX + 239, y, 685, 41};
    
    if (isFirst) {
        // Gold gradient for first place
        // Simplified: use solid gold color
        SDL_SetRenderDrawColor(r, 0x8c, 0x58, 0x2a, 255);
    } else {
        // Brown gradient for others
        SDL_SetRenderDrawColor(r, 0x63, 0x48, 0x2c, 255);
    }
    SDL_RenderFillRect(r, &rowRect);
    
    // Render rank number, name, WPM, accuracy
    TTF_Font* dataFont = app->resources().font(UiTheme::MainFontPath, 24);
    if (!dataFont) return;
    
    // Rank: "1.", "2.", etc.
    std::string rankStr = std::to_string(data.rank) + ".";
    drawText(r, dataFont, rankStr, frameX + 251, y + 10, UiTheme::White);
    
    // Player name
    drawText(r, dataFont, data.display_name, frameX + 326, y + 12, UiTheme::White);
    
    // WPM
    std::ostringstream wpmStream;
    wpmStream << (int)data.wpm;
    drawText(r, dataFont, wpmStream.str(), frameX + 767, y + 10, UiTheme::White);
    
    // Accuracy (as percentage)
    std::ostringstream accStream;
    accStream << (int)data.accuracy << "%";
    drawText(r, dataFont, accStream.str(), frameX + 861, y + 10, UiTheme::White);
}

void MatchResultOverlay::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int ww, wh;
    SDL_GetWindowSize(SDL_GetWindowFromID(1), &ww, &wh);
    
    // Calculate scaling
    float scaleX = (float)ww / UiTheme::DesignW;
    float scaleY = (float)wh / UiTheme::DesignH;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;
    
    int offsetX = (ww - (int)(UiTheme::DesignW * scale)) / 2;
    int offsetY = (wh - (int)(UiTheme::DesignH * scale)) / 2;
    
    lx = (int)((wx - offsetX) / scale);
    ly = (int)((wy - offsetY) / scale);
}

void MatchResultOverlay::handleExitMenu() {
    std::cout << "[MatchResultOverlay] Exit menu pressed\n";
    
    // Exit to menu: close overlay, close GameScreen, close LobbyScreen
    // Send exit_room to server
    // Navigate to TitleScreen
    
    app->defer([this]() {
        // Send exit_room command
        app->network().send_exit_room();
        
        // Clear game and room state
        app->state().clearGameInit();
        app->state().clearGameState();
        app->state().clearGameEnd();
        app->state().clearRoomState();
        
        // Navigate to title screen (replace all views)
        app->router().change(RouteId::Title);
    });
}

void MatchResultOverlay::handleReturnLobby() {
    std::cout << "[MatchResultOverlay] Return lobby pressed\n";
    
    // Return to lobby: close overlay and GameScreen
    // Keep room, server will send updated room_state
    
    app->defer([this]() {
        // Clear game state but keep room
        app->state().clearGameInit();
        app->state().clearGameState();
        app->state().clearGameEnd();
        
        // Pop overlay and GameScreen to return to LobbyScreen
        app->router().pop(); // pop overlay
        app->router().pop(); // pop GameScreen
        
        // Server will automatically send room_state when game ends
        // and players return to lobby
    });
}
