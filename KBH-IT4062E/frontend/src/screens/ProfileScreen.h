#pragma once
#include "../core/View.h"
#include <SDL.h>
#include <string>
#include <vector>
#include <iostream>

class ProfileScreen : public View {
public:
    ProfileScreen() 
        : bg(nullptr), logo(nullptr), 
          logoW(687), logoH(518),
          btnW(612), btnH(74), btnRadius(20), btnBorder(4),
          hoveredMenu(-1), hoveredSignOut(false) 
    {
        std::cout << "[ProfileScreen] Constructor called\n";
        signOutRect = SDL_Rect{0,0,0,0};
        signOutText.tex = nullptr;
        signOutText.w = 0;
        signOutText.h = 0;
    }
    
    void onEnter() override;
    void onExit() override;

    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    struct TextTex {
        SDL_Texture* tex = nullptr;
        int w = 0;
        int h = 0;
        std::string text;
    };

    // text cache
    TextTex makeText(const std::string& s, int fontSize);
    void destroyText(TextTex& t);
    void drawTextShadow(SDL_Renderer* r, const TextTex& t, int x, int y, SDL_Color mainColor);

    // hover button draw - useRedColor for Sign Out button
    void drawHoverButton(SDL_Renderer* r, const SDL_Rect& outerRect, bool useRedColor);

    // input mapping (window -> logical)
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void updateHoverFromMouse(int wx, int wy);

private:
    SDL_Texture* bg = nullptr;
    SDL_Texture* logo = nullptr;

    // layout
    int logoW = 687;
    int logoH = 518;

    // figma button geometry
    int btnW = 612;
    int btnH = 74;
    int btnRadius = 20;
    int btnBorder = 4;

    // menu items (populated in onEnter)
    std::vector<TextTex> menuText;
    std::vector<SDL_Rect> menuRect;

    TextTex signOutText;
    SDL_Rect signOutRect{0,0,0,0};

    int hoveredMenu = -1;
    bool hoveredSignOut = false;
};
