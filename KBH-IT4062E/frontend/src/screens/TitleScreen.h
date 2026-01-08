#pragma once
#include "../core/View.h"
#include <SDL.h>
#include <string>
#include <vector>

class TitleScreen : public View {
public:
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

    // hover button draw
    void drawHoverButton(SDL_Renderer* r, const SDL_Rect& outerRect);

    // input mapping (window -> logical)
    void windowToLogical(int wx, int wy, int& lx, int& ly) const;
    void updateHoverFromMouse(int wx, int wy);

private:
    SDL_Texture* bg = nullptr;
    SDL_Texture* logo = nullptr;

    // layout
    int logoW = 687;
    int logoH = 518;

    // figma button geometry (từ CSS)
    int btnW = 612;
    int btnH = 74;
    int btnRadius = 20;
    int btnBorder = 4;

    // menu items
    std::vector<std::string> labels {
        "Create Room",
        "Join Room",
        "Training",
        "Leaderboard"
    };

    std::vector<TextTex> menuText;
    std::vector<SDL_Rect> menuRect; // clickable/hover area = button rect

    TextTex signText;
    SDL_Rect signRect{0,0,0,0};

    int hoveredMenu = -1;
    bool hoveredSign = false;
};
