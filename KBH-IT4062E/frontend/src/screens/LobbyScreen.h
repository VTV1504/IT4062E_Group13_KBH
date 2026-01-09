#pragma once
#include "../core/View.h"
#include "../net/NetEvents.h"
#include <SDL_ttf.h>

class LobbyScreen : public View {
public:
    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* r) override;

private:
    // Textures
    SDL_Texture* logoKnight = nullptr;
    SDL_Texture* privateIcon = nullptr;
    SDL_Texture* readyCheck = nullptr;
    SDL_Texture* copyIcon = nullptr;
    SDL_Texture* knightTextures[8] = {nullptr};
    
    // Button hit rects
    SDL_Rect exitRect;
    SDL_Rect readyRect;
    SDL_Rect startRect;
    SDL_Rect copyRoomIdRect;
    SDL_Rect togglePrivateRect;
    
    int hoveredBtn = -1; // 0=exit, 1=ready, 2=start, 3=copy, 4=private
    
    // Helper functions
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void renderPlayerCard(SDL_Renderer* r, const RoomSlotData& slot, int x, int y, bool isHighlight);
    void renderEmptyCard(SDL_Renderer* r, int x, int y);
    void renderFooter(SDL_Renderer* r);
    void renderPrivateState(SDL_Renderer* r);
    
    // Button callbacks (placeholder)
    void btn_exit_pressed();
    void btn_ready_pressed();
    void btn_start_pressed();
    void btn_copy_room_id_pressed();
    void btn_toggle_private_pressed();
    
    // Check if self is host
    bool isSelfHost() const;
    bool isSelfReady() const;
};
