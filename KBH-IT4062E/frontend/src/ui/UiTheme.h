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

    // ====== Lobby screen file paths
    static constexpr const char* LobbyBgKnightPath = "res/images/logo_knight.png";   // big faint knight
    static constexpr const char* LobbyReadyTickPath = "res/images/ready.png"; // 73x73 tick
    static constexpr const char* LobbyLockPath = "res/images/private_small.png";     // 76x84 lock
    static constexpr const char* LobbyCopyIconPath = "res/images/copy.png";            // optional

    static constexpr const char* LobbyKnightPaths[8] = {
        "res/images/lightblue_full.png",
        "res/images/black_full.png",
        "res/images/brown_full.png",
        "res/images/red_full.png",
        "res/images/blue_full.png",
        "res/images/pink_full.png",
        "res/images/spartan_full.png",
        "res/images/yellow_full.png"
    };

    static constexpr SDL_Color White {255,255,255,255};
    static constexpr SDL_Color Warm  {255,245,200,255};
    static constexpr SDL_Color Yellow{255,230, 40,255};
}
