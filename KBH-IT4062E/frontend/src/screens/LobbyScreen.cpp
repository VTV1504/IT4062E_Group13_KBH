#include "LobbyScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <iostream>

static bool pointInRect(int x, int y, const SDL_Rect& r) {
    return (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
}

void LobbyScreen::onEnter() {
    std::cout << "[LobbyScreen] onEnter() called\n";
    
    // Load assets with null checks
    logoKnight = app->resources().texture(UiTheme::LobbyBgKnightPath);
    if (!logoKnight) std::cerr << "[LobbyScreen] Failed to load logoKnight\n";
    
    privateIcon = app->resources().texture(UiTheme::LobbyLockPath);
    if (!privateIcon) std::cerr << "[LobbyScreen] Failed to load privateIcon\n";
    
    readyCheck = app->resources().texture(UiTheme::LobbyReadyTickPath);
    if (!readyCheck) std::cerr << "[LobbyScreen] Failed to load readyCheck\n";
    
    copyIcon = app->resources().texture(UiTheme::LobbyCopyIconPath);
    if (!copyIcon) std::cerr << "[LobbyScreen] Failed to load copyIcon\n";
    
    // Load all knight textures
    std::cout << "[LobbyScreen] Loading knight textures...\n";
    for (int i = 0; i < 8; ++i) {
        knightTextures[i] = app->resources().texture(UiTheme::LobbyKnightPaths[i]);
        if (!knightTextures[i]) {
            std::cerr << "[LobbyScreen] Failed to load knight texture " << i << ": " 
                      << UiTheme::LobbyKnightPaths[i] << "\n";
        }
    }
    
    std::cout << "[LobbyScreen] Setting up button rects...\n";
    // Setup button rects (footer y = 1024 - 150 = 874)
    const int footerY = 874;
    exitRect = {27, footerY + 46, 250, 60};
    readyRect = {972, footerY + 46, 250, 60};
    startRect = {1259, footerY + 46, 250, 60};
    
    // Room ID copy button (top right area, near room ID display)
    copyRoomIdRect = {980, 90, 50, 50};
    
    // Toggle private button (near private indicator)
    togglePrivateRect = {1358, 30, 113, 100};
    
    hoveredBtn = -1;
    
    std::cout << "[LobbyScreen] onEnter() complete\n";
}

void LobbyScreen::onExit() {
    std::cout << "[LobbyScreen] onExit() called\n";
    logoKnight = privateIcon = readyCheck = copyIcon = nullptr;
    for (int i = 0; i < 8; ++i) {
        knightTextures[i] = nullptr;
    }
    std::cout << "[LobbyScreen] onExit() complete\n";
}

void LobbyScreen::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(app->renderer(), &outW, &outH);
    if (outW <= 0) outW = UiTheme::DesignW;
    if (outH <= 0) outH = UiTheme::DesignH;
    
    lx = wx * UiTheme::DesignW / outW;
    ly = wy * UiTheme::DesignH / outH;
}

void LobbyScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        int mx, my;
        windowToLogical(e.motion.x, e.motion.y, mx, my);
        
        hoveredBtn = -1;
        if (pointInRect(mx, my, exitRect)) hoveredBtn = 0;
        else if (pointInRect(mx, my, readyRect)) hoveredBtn = 1;
        else if (pointInRect(mx, my, startRect)) {
            // Only hover if can start
            if (isSelfHost() && app->state().hasRoom() && app->state().getRoomState().can_start) {
                hoveredBtn = 2;
            }
        }
        else if (pointInRect(mx, my, copyRoomIdRect)) hoveredBtn = 3;
        else if (pointInRect(mx, my, togglePrivateRect) && isSelfHost()) hoveredBtn = 4;
        return;
    }
    
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx, my;
        windowToLogical(e.button.x, e.button.y, mx, my);
        
        if (pointInRect(mx, my, exitRect)) {
            btn_exit_pressed();
        } else if (pointInRect(mx, my, readyRect)) {
            btn_ready_pressed();
        } else if (pointInRect(mx, my, startRect)) {
            if (isSelfHost() && app->state().hasRoom() && app->state().getRoomState().can_start) {
                btn_start_pressed();
            }
        } else if (pointInRect(mx, my, copyRoomIdRect)) {
            btn_copy_room_id_pressed();
        } else if (pointInRect(mx, my, togglePrivateRect) && isSelfHost()) {
            btn_toggle_private_pressed();
        }
        return;
    }
    
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        btn_exit_pressed();
    }
}

void LobbyScreen::update(float) {
    // Nothing to update per frame
}

void LobbyScreen::render(SDL_Renderer* r) {
    // Background: rgba(45, 44, 71, 0.96)
    SDL_SetRenderDrawColor(r, 45, 44, 71, 245);
    SDL_RenderClear(r);
    
    // Top separator line at y=162, x=64..1473, thickness 3
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect sepTop{64, 162, 1409, 3};
    SDL_RenderFillRect(r, &sepTop);
    
    // Logo decoration (background)
    if (logoKnight) {
        SDL_Rect dst{807, 296, 1027, 841};
        SDL_RenderCopy(r, logoKnight, nullptr, &dst);
    }
    
    // Room ID display (top center)
    if (app->state().hasRoom()) {
        const auto& room = app->state().getRoomState();
        TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
        if (f && !room.room_id.empty()) {
            // Safe string handling
            std::string safeRoomId = room.room_id;
            if (safeRoomId.length() > 20) safeRoomId = safeRoomId.substr(0, 20);
            std::string roomText = "Room ID: " + safeRoomId;
            int textX = 500;
            int textY = 95;
            drawText(r, f, roomText, textX, textY, UiTheme::White);
            
            // Copy button (icon)
            if (copyIcon) {
                SDL_SetTextureColorMod(copyIcon, 
                    hoveredBtn == 3 ? 255 : 200,
                    hoveredBtn == 3 ? 245 : 200,
                    hoveredBtn == 3 ? 200 : 200);
                SDL_RenderCopy(r, copyIcon, nullptr, &copyRoomIdRect);
                SDL_SetTextureColorMod(copyIcon, 255, 255, 255);
            }
        }
    }
    
    // Private state indicator and toggle button
    renderPrivateState(r);
    
    // Player grid (8 slots, 2 cols x 4 rows)
    const int gridX = 137;
    const int gridY = 190;
    const int colGap = 697;
    const int rowOffsets[] = {-4, 169, 342, 515};
    
    if (!app->state().hasRoom()) {
        // No room state yet
        TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 44);
        drawText(r, f, "Waiting for room...", gridX, gridY, UiTheme::White);
    } else {
        const auto& room = app->state().getRoomState();
        
        for (int i = 0; i < 8; ++i) {
            int col = i / 4;
            int row = i % 4;
            int x = gridX + col * colGap;
            int y = gridY + rowOffsets[row];
            
            const auto& slot = room.slots[i];
            
            if (slot.occupied) {
                // Highlight if this is self
                bool highlight = (slot.client_id == room.self_client_id);
                renderPlayerCard(r, slot, x, y, highlight);
            } else {
                renderEmptyCard(r, x, y);
            }
        }
    }
    
    // Footer bar
    renderFooter(r);
}

void LobbyScreen::renderPlayerCard(SDL_Renderer* r, const RoomSlotData& slot, int x, int y, bool isHighlight) {
    const int w = 570;
    const int h = 141;
    const int borderThick = 8;
    
    // Shadow (fake): offset (0,5), blur color black alpha
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 128);
    SDL_Rect shadow{x, y + 5, w, h};
    SDL_RenderFillRect(r, &shadow);
    
    // Border
    SDL_Rect borderRect{x, y, w, h};
    if (isHighlight) {
        SDL_SetRenderDrawColor(r, 255, 175, 0, 194); // rgba(255,175,0,0.76)
    } else {
        SDL_SetRenderDrawColor(r, 203, 202, 217, 245); // rgba(203, 202, 217, 0.96)
    }
    SDL_RenderFillRect(r, &borderRect);
    
    // Fill
    SDL_Rect fillRect{x + borderThick, y + borderThick, w - borderThick * 2, h - borderThick * 2};
    if (isHighlight) {
        SDL_SetRenderDrawColor(r, 126, 93, 20, 255); // #7E5D14
    } else {
        SDL_SetRenderDrawColor(r, 41, 44, 74, 255); // #292C4A
    }
    SDL_RenderFillRect(r, &fillRect);
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    
    // Knight avatar icon (left side, scaled to 144x144)
    int knightIdx = slot.knight_idx;
    if (knightIdx >= 0 && knightIdx < 8 && knightTextures[knightIdx]) {
        SDL_Rect avatarRect{x + 5, y + (h - 144) / 2, 144, 144};
        SDL_RenderCopy(r, knightTextures[knightIdx], nullptr, &avatarRect);
    }
    
    // Player info
    
    // Name text
    TTF_Font* nameFont = app->resources().font(UiTheme::MainFontPath, 37);
    if (nameFont && !slot.display_name.empty()) {
        int textX = x + 160;
        int textY = y + 20;
        // Safe string copy to prevent heap corruption
        std::string safeName = slot.display_name;
        if (safeName.length() > 50) safeName = safeName.substr(0, 50); // Limit length
        drawText(r, nameFont, safeName, textX, textY, UiTheme::White);
        
        // Host label
        if (slot.is_host) {
            TTF_Font* hostFont = app->resources().font(UiTheme::MainFontPath, 30);
            if (hostFont) {
                SDL_Color hostColor{255, 175, 0, 255}; // #FFAF00
                drawText(r, hostFont, "( Host )", textX, textY + 45, hostColor);
            }
        }
    }
    
    // Ready check icon (right side)
    if (readyCheck && slot.is_ready) {
        const int iconSize = 73;
        SDL_Rect iconRect{x + w - iconSize - 20, y + (h - iconSize) / 2, iconSize, iconSize};
        SDL_RenderCopy(r, readyCheck, nullptr, &iconRect);
    }
}

void LobbyScreen::renderEmptyCard(SDL_Renderer* r, int x, int y) {
    const int w = 570;
    const int h = 141;
    const int borderThick = 8;
    
    // Shadow
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 128);
    SDL_Rect shadow{x, y + 5, w, h};
    SDL_RenderFillRect(r, &shadow);
    
    // Border
    SDL_Rect borderRect{x, y, w, h};
    SDL_SetRenderDrawColor(r, 203, 202, 217, 245);
    SDL_RenderFillRect(r, &borderRect);
    
    // Fill
    SDL_Rect fillRect{x + borderThick, y + borderThick, w - borderThick * 2, h - borderThick * 2};
    SDL_SetRenderDrawColor(r, 41, 44, 74, 255);
    SDL_RenderFillRect(r, &fillRect);
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    
    // Horizontal dash in center: width 74, height 13, color #C5C4D3
    SDL_SetRenderDrawColor(r, 197, 196, 211, 255);
    SDL_Rect dash{x + (w - 74) / 2, y + (h - 13) / 2, 74, 13};
    SDL_RenderFillRect(r, &dash);
}

void LobbyScreen::renderFooter(SDL_Renderer* r) {
    const int footerY = 874;
    const int footerH = 150;
    
    // Footer bar: rgba(40, 44, 74, 0.7)
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 40, 44, 74, 179);
    SDL_Rect footerBg{0, footerY, 1536, footerH};
    SDL_RenderFillRect(r, &footerBg);
    
    // Top separator line
    SDL_SetRenderDrawColor(r, 194, 194, 194, 255);
    SDL_Rect sepLine{0, footerY + 1, 1536, 3};
    SDL_RenderFillRect(r, &sepLine);
    
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    
    // Buttons text (underlined)
    TTF_Font* btnFont = app->resources().font(UiTheme::MainFontPath, 48);
    if (!btnFont) return;
    
    // Exit button
    SDL_Color exitColor = (hoveredBtn == 0) ? UiTheme::Warm : UiTheme::White;
    drawText(r, btnFont, "Exit", exitRect.x, exitRect.y, exitColor);
    
    // Ready button
    SDL_Color readyColor = (hoveredBtn == 1) ? UiTheme::Warm : UiTheme::White;
    std::string readyLabel = isSelfReady() ? "Unready" : "Ready";
    drawText(r, btnFont, readyLabel, readyRect.x, readyRect.y, readyColor);
    
    // Start button (only if host and can start)
    bool canStart = isSelfHost() && app->state().hasRoom() && app->state().getRoomState().can_start;
    SDL_Color startColor;
    if (canStart) {
        startColor = (hoveredBtn == 2) ? UiTheme::Warm : UiTheme::White;
    } else {
        startColor = {128, 128, 128, 128}; // disabled gray
    }
    drawText(r, btnFont, "Start", startRect.x, startRect.y, startColor);
    
    // Underlines (simple line below text)
    SDL_SetRenderDrawColor(r, exitColor.r, exitColor.g, exitColor.b, exitColor.a);
    SDL_Rect exitLine{exitRect.x, exitRect.y + 55, 100, 2};
    SDL_RenderFillRect(r, &exitLine);
    
    SDL_SetRenderDrawColor(r, readyColor.r, readyColor.g, readyColor.b, readyColor.a);
    SDL_Rect readyLine{readyRect.x, readyRect.y + 55, 150, 2};
    SDL_RenderFillRect(r, &readyLine);
    
    if (canStart) {
        SDL_SetRenderDrawColor(r, startColor.r, startColor.g, startColor.b, startColor.a);
        SDL_Rect startLine{startRect.x, startRect.y + 55, 120, 2};
        SDL_RenderFillRect(r, &startLine);
    }
}

void LobbyScreen::renderPrivateState(SDL_Renderer* r) {
    if (!app->state().hasRoom()) return;
    
    const auto& room = app->state().getRoomState();
    
    const int x = 1358;
    const int y = 30;
    const int iconSize = 76;
    
    // Determine opacity based on state
    Uint8 opacity = 255; // Default 100%
    if (room.is_private) {
        opacity = 255; // Private: 100%
    } else if (hoveredBtn == 4 && isSelfHost()) {
        opacity = 191; // Hover: 75% (0.75 * 255 = 191)
    } else {
        opacity = 51;  // Public: 20% (0.2 * 255 = 51)
    }
    
    // No background - removed to make it cleaner
    
    // Lock icon - always show with opacity
    if (privateIcon) {
        SDL_SetTextureAlphaMod(privateIcon, opacity);
        SDL_Rect iconRect{x + (113 - iconSize) / 2, y, iconSize, iconSize};
        SDL_RenderCopy(r, privateIcon, nullptr, &iconRect);
        SDL_SetTextureAlphaMod(privateIcon, 255); // Reset
    }
    
    // Text "PRIVATE" - always show with opacity
    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, 25);
    if (f) {
        SDL_Color c{255, 255, 255, opacity};
        std::string labelText = "PRIVATE";
        drawText(r, f, labelText, x, y + iconSize - 7, c);
    }
}

// Button callbacks (placeholder)
void LobbyScreen::btn_exit_pressed() {
    std::cout << "[LobbyScreen] Exit button pressed\n";
    app->network().send_exit_room();
    app->state().clearRoomState();
    app->router().change(RouteId::Title);
}

void LobbyScreen::btn_ready_pressed() {
    std::cout << "[LobbyScreen] Ready button pressed\n";
    
    if (isSelfReady()) {
        app->network().send_unready();
    } else {
        app->network().send_ready();
    }
}

void LobbyScreen::btn_start_pressed() {
    std::cout << "[LobbyScreen] Start button pressed\n";
    app->network().send_start_game(60000); // 60 seconds
}

bool LobbyScreen::isSelfHost() const {
    if (!app->state().hasRoom()) return false;
    
    const auto& room = app->state().getRoomState();
    for (int i = 0; i < 8; ++i) {
        if (room.slots[i].occupied && room.slots[i].client_id == room.self_client_id) {
            return room.slots[i].is_host;
        }
    }
    return false;
}

bool LobbyScreen::isSelfReady() const {
    if (!app->state().hasRoom()) return false;
    
    const auto& room = app->state().getRoomState();
    for (int i = 0; i < 8; ++i) {
        if (room.slots[i].occupied && room.slots[i].client_id == room.self_client_id) {
            return room.slots[i].is_ready;
        }
    }
    return false;
}

void LobbyScreen::btn_copy_room_id_pressed() {
    if (!app->state().hasRoom()) return;
    
    const auto& room = app->state().getRoomState();
    if (room.room_id.empty()) return;
    
    std::cout << "[LobbyScreen] Copy room ID: " << room.room_id << "\n";
    
    // Copy to clipboard using SDL - make safe copy first
    std::string safeId = room.room_id;
    SDL_SetClipboardText(safeId.c_str());
}

void LobbyScreen::btn_toggle_private_pressed() {
    if (!isSelfHost()) return;
    if (!app->state().hasRoom()) return;
    
    const auto& room = app->state().getRoomState();
    bool newPrivateState = !room.is_private;
    
    std::cout << "[LobbyScreen] Toggle private/public pressed - setting to: " 
              << (newPrivateState ? "private" : "public") << "\n";
    
    // Send message to server
    app->network().send_set_private(newPrivateState);
}
