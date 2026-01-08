#pragma once
#include <SDL.h>

namespace UiTheme {
    static constexpr int DesignW = 1536;
    static constexpr int DesignH = 1024;

    // đổi cho đúng tên file của bạn
    static constexpr const char* TitleBgPath   = "res/images/title_bg.png";
    static constexpr const char* TitleLogoPath = "res/images/logo_fin.png";
    static constexpr const char* MainFontPath  = "res/fonts/Berylium Bd.otf";
    static constexpr const char* SubFontPath  = "res/fonts/dosis.medium.ttf";

    static constexpr SDL_Color White {255,255,255,255};
    static constexpr SDL_Color Warm  {255,245,200,255};
    static constexpr SDL_Color Yellow{255,230, 40,255};
}
