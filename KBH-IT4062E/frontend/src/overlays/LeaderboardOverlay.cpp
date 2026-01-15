#include "LeaderboardOverlay.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

LeaderboardOverlay::LeaderboardOverlay()
    : scrollOffset(0), maxScroll(0) {
    // Position from TitleScreen: x=855, y=223
    panelRect = {855, 223, 680, 736};
}

void LeaderboardOverlay::onEnter() {
    std::cout << "[LeaderboardOverlay] onEnter()\n";
    scrollOffset = 0;
    
    // Calculate max scroll based on number of entries
    if (app && app->state().hasLeaderboard()) {
        const auto& lb = app->state().getLeaderboard();
        int numEntries = (int)lb.top8.size();
        int visibleRows = 426 / 57;  // Container height / row height
        maxScroll = (numEntries > visibleRows) ? (numEntries - visibleRows) : 0;
        std::cout << "[LeaderboardOverlay] Loaded " << numEntries << " entries, maxScroll=" << maxScroll << "\n";
    } else {
        maxScroll = 0;
        std::cout << "[LeaderboardOverlay] No leaderboard data available\n";
    }
}

void LeaderboardOverlay::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(app->renderer(), &outW, &outH);
    if (outW <= 0) outW = UiTheme::DesignW;
    if (outH <= 0) outH = UiTheme::DesignH;
    
    lx = wx * UiTheme::DesignW / outW;
    ly = wy * UiTheme::DesignH / outH;
}

void LeaderboardOverlay::handleEvent(const SDL_Event& e) {
    // ESC to close
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().pop();
        return;
    }
    
    // Click outside to close
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int wx = e.button.x;
        int wy = e.button.y;
        int lx, ly;
        windowToLogical(wx, wy, lx, ly);
        
        // Check if click is outside panel
        if (lx < panelRect.x || lx >= panelRect.x + panelRect.w ||
            ly < panelRect.y || ly >= panelRect.y + panelRect.h) {
            app->router().pop();
            return;
        }
    }
    
    // Mouse wheel scrolling
    if (e.type == SDL_MOUSEWHEEL) {
        if (e.wheel.y > 0) {
            // Scroll up
            scrollOffset = std::max(0, scrollOffset - 1);
        } else if (e.wheel.y < 0) {
            // Scroll down
            scrollOffset = std::min(maxScroll, scrollOffset + 1);
        }
    }
}

void LeaderboardOverlay::render(SDL_Renderer* r) {
    if (!app) return;
    
    // Semi-transparent background overlay
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
    SDL_Rect fullScreen = {0, 0, UiTheme::DesignW, UiTheme::DesignH};
    SDL_RenderFillRect(r, &fullScreen);
    
    int x = panelRect.x;
    int y = panelRect.y;
    
    // Main panel background
    SDL_SetRenderDrawColor(r, 133, 113, 69, (Uint8)(0.93 * 255));
    SDL_Rect panel = {x, y, 680, 736};
    SDL_RenderFillRect(r, &panel);
    
    // Left border gradient
    SDL_SetRenderDrawColor(r, 255, 166, 102, 255);
    SDL_Rect leftBorder = {x, y, 10, 736};
    SDL_RenderFillRect(r, &leftBorder);
    
    // Title
    TTF_Font* titleFont = app->resources().font(UiTheme::MainFontPath, 53);
    if (titleFont) {
        SDL_Color white = {255, 255, 255, 255};
        drawText(r, titleFont, "LEADERBOARD", x + 158, y + 16, white);
    }
    
    // Horizontal line
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect hLine = {x + 59, y + 90, 562, 5};
    SDL_RenderFillRect(r, &hLine);
    
    // Row header at (62, 124)
    int headerY = y + 124;
    TTF_Font* font = app->resources().font(UiTheme::MainFontPath, 37);
    if (font) {
        SDL_Color white = {255, 255, 255, 255};
        drawText(r, font, "No.", x + 72, headerY, white);
        drawText(r, font, "Username", x + 225, headerY, white);
        drawText(r, font, "WPM", x + 555, headerY, white);
    }
    
    // Scrollable entries container at (65, 181) size 554x426
    int containerX = x + 65;
    int containerY = y + 181;
    int containerW = 554;
    int containerH = 426;
    
    // Get leaderboard data
    std::vector<LeaderboardEntry> entries;
    if (app->state().hasLeaderboard()) {
        const auto& lb = app->state().getLeaderboard();
        entries = lb.top8;
    }
    
    // Set clipping rect for scrollable area
    SDL_Rect clipRect = {containerX, containerY, containerW, containerH};
    SDL_RenderSetClipRect(r, &clipRect);
    
    // Draw entries with scroll offset
    int rowHeight = 57;
    for (size_t i = 0; i < entries.size(); i++) {
        int rowY = containerY + (i - scrollOffset) * rowHeight;
        
        // Skip if out of visible area
        if (rowY + rowHeight < containerY || rowY >= containerY + containerH) {
            continue;
        }
        
        const auto& entry = entries[i];
        
        // Alternating background color for odd rows
        if (i % 2 == 0) {
            SDL_SetRenderDrawColor(r, 171, 167, 116, (Uint8)(0.95 * 255));
            SDL_Rect rowBg = {containerX, rowY, containerW, rowHeight};
            SDL_RenderFillRect(r, &rowBg);
        }
        
        // Draw rank, username, wpm
        if (font) {
            SDL_Color white = {255, 255, 255, 255};
            
            // Rank (center at ~72)
            std::ostringstream rankStr;
            rankStr << entry.rank;
            drawText(r, font, rankStr.str(), containerX + 7, rowY + 10, white);
            
            // Username (left align at ~140)
            drawText(r, font, entry.username, containerX + 75, rowY + 10, white);
            
            // WPM (right align at ~490)
            std::ostringstream wpmStr;
            wpmStr << std::fixed << std::setprecision(1) << entry.wpm;
            drawText(r, font, wpmStr.str(), containerX + 435, rowY + 10, white);
        }
    }
    
    // Reset clip rect
    SDL_RenderSetClipRect(r, nullptr);
    
    // Draw border around scrollable area
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect borderRect = {containerX, containerY, containerW, containerH};
    // Top
    SDL_RenderDrawLine(r, borderRect.x, borderRect.y, borderRect.x + borderRect.w, borderRect.y);
    // Bottom
    SDL_RenderDrawLine(r, borderRect.x, borderRect.y + borderRect.h, borderRect.x + borderRect.w, borderRect.y + borderRect.h);
    // Left
    SDL_RenderDrawLine(r, borderRect.x, borderRect.y, borderRect.x, borderRect.y + borderRect.h);
    // Right
    SDL_RenderDrawLine(r, borderRect.x + borderRect.w, borderRect.y, borderRect.x + borderRect.w, borderRect.y + borderRect.h);
    
    // Self rank at (61, 612) - only if user has rank
    if (app->state().hasLeaderboard()) {
        const auto& lb = app->state().getLeaderboard();
        const auto& selfRank = lb.self_rank;
        
        if (selfRank.rank > 0) {  // Only draw if user has a rank
            int selfY = y + 612;
            
            // Background
            SDL_SetRenderDrawColor(r, 171, 167, 116, (Uint8)(0.95 * 255));
            SDL_Rect selfBg = {x + 61, selfY, 562, 57};
            SDL_RenderFillRect(r, &selfBg);
            
            // Gold border (4px)
            SDL_SetRenderDrawColor(r, 255, 175, 0, (Uint8)(0.76 * 255));
            for (int i = 0; i < 4; i++) {
                SDL_Rect borderLine = {selfBg.x - i, selfBg.y - i, selfBg.w + i*2, selfBg.h + i*2};
                SDL_RenderDrawRect(r, &borderLine);
            }
            
            // Draw self rank data
            if (font) {
                SDL_Color white = {255, 255, 255, 255};
                
                std::ostringstream rankStr;
                rankStr << selfRank.rank;
                drawText(r, font, rankStr.str(), x + 72, selfY + 10, white);
                
                drawText(r, font, selfRank.username, x + 140, selfY + 10, white);
                
                std::ostringstream wpmStr;
                wpmStr << std::fixed << std::setprecision(1) << selfRank.wpm;
                drawText(r, font, wpmStr.str(), x + 500, selfY + 10, white);
            }
        }
    }
}
