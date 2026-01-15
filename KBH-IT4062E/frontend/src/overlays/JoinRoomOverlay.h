#pragma once
#include "../core/View.h"
#include <SDL_ttf.h>
#include <string>

class JoinRoomOverlay : public View {
public:
    void onEnter() override;
    void onExit() override;
    void onResume() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* r) override;
    
    // Public API for external control
    void set_join_result(bool ok, const std::string& msg);
    void set_error(const std::string& msg);
    void clear_error();

private:
    // State
    std::string room_code;
    std::string error_msg;
    bool show_error = false;
    
    // UI state
    int hoveredBtn = -1; // 0=back, 1=join, 2=random
    
    // Helper functions
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void drawRoundedRect(SDL_Renderer* r, int x, int y, int w, int h, int radius, SDL_Color fill);
    bool isJoinEnabled() const;
    
    // Button callbacks (placeholders)
    void btn_back_to_menu_pressed();
    void btn_join_pressed();
    void btn_random_room_pressed();
    
    // Hit rects
    SDL_Rect backLinkRect;
    SDL_Rect joinBtnRect;
    SDL_Rect randomBtnRect;
    SDL_Rect inputRect;
};
