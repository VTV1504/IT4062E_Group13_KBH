#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "UiTheme.h"

inline void drawText(SDL_Renderer* r, TTF_Font* font, const std::string& text,
                     int x, int y, SDL_Color color) {
    if (!font) return;
    SDL_Surface* s = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect dst{ x, y, s->w, s->h };
    SDL_FreeSurface(s);
    if (t) {
        SDL_RenderCopy(r, t, nullptr, &dst);
        SDL_DestroyTexture(t);
    }
}

inline void drawOverlayDim(SDL_Renderer* r, Uint8 alpha = 160) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, alpha);
    SDL_Rect full{0,0,UiTheme::DesignW,UiTheme::DesignH};
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
