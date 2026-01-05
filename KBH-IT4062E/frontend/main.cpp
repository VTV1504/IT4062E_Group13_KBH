#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>

static const int kDesignW = 1366;
static const int kDesignH = 768;

static const char* kBgPath   = "res/images/title_bg.png";
static const char* kLogoPath = "res/images/logo.png";
static const char* kFontPath = "res/fonts/Berylium Bd.otf";

struct Texture {
    SDL_Texture* t = nullptr;
    int w = 0, h = 0;
};

static Texture loadTexture(SDL_Renderer* r, const char* path) {
    Texture out{};
    SDL_Surface* s = IMG_Load(path);
    if (!s) {
        std::fprintf(stderr, "IMG_Load failed (%s): %s\n", path, IMG_GetError());
        return out;
    }
    out.w = s->w;
    out.h = s->h;
    out.t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    if (!out.t) {
        std::fprintf(stderr, "CreateTextureFromSurface failed (%s): %s\n", path, SDL_GetError());
    }
    return out;
}

static Texture renderText(SDL_Renderer* r, TTF_Font* font, const std::string& text, SDL_Color color) {
    Texture out{};
    SDL_Surface* s = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!s) {
        std::fprintf(stderr, "TTF_RenderUTF8_Blended failed: %s\n", TTF_GetError());
        return out;
    }
    out.w = s->w;
    out.h = s->h;
    out.t = SDL_CreateTextureFromSurface(r, s);
    SDL_FreeSurface(s);
    if (!out.t) {
        std::fprintf(stderr, "CreateTextureFromSurface(text) failed: %s\n", SDL_GetError());
    }
    return out;
}

static void destroy(Texture& x) {
    if (x.t) SDL_DestroyTexture(x.t);
    x.t = nullptr;
    x.w = x.h = 0;
}

static void drawTextWithShadow(SDL_Renderer* r, const Texture& textTex, int x, int y, int shadowDx, int shadowDy) {
    if (!textTex.t) return;

    SDL_SetTextureColorMod(textTex.t, 0, 0, 0);
    SDL_Rect shadow{ x + shadowDx, y + shadowDy, textTex.w, textTex.h };
    SDL_RenderCopy(r, textTex.t, nullptr, &shadow);

    SDL_SetTextureColorMod(textTex.t, 255, 255, 255);
    SDL_Rect dst{ x, y, textTex.w, textTex.h };
    SDL_RenderCopy(r, textTex.t, nullptr, &dst);
}

static void drawTexture(SDL_Renderer* r, const Texture& tex, int x, int y, int w = -1, int h = -1) {
    if (!tex.t) return;
    SDL_Rect dst{ x, y, (w < 0 ? tex.w : w), (h < 0 ? tex.h : h) };
    SDL_RenderCopy(r, tex.t, nullptr, &dst);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        std::fprintf(stderr, "IMG_Init PNG failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() != 0) {
        std::fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow(
        "Keyboard Hero",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kDesignW, kDesignH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) {
        std::fprintf(stderr, "CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        std::fprintf(stderr, "CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(ren, kDesignW, kDesignH);

    Texture bg = loadTexture(ren, kBgPath);
    Texture logo = loadTexture(ren, kLogoPath);

    TTF_Font* fontMenu = TTF_OpenFont(kFontPath, 50);
    TTF_Font* fontSign = TTF_OpenFont(kFontPath, 44);
    if (!fontMenu || !fontSign) {
        std::fprintf(stderr, "TTF_OpenFont failed (%s): %s\n", kFontPath, TTF_GetError());
        destroy(bg);
        destroy(logo);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    struct MenuItem {
        std::string label;
        Texture normal;
        Texture selected;
    };

    std::vector<std::string> labels = {
        "Create Room",
        "Join Room",
        "Training",
        "Leaderboard"
    };

    SDL_Color white{255,255,255,255};
    SDL_Color warm{255,245,200,255};

    std::vector<MenuItem> menu;
    menu.reserve(labels.size());
    for (auto& s : labels) {
        MenuItem it;
        it.label = s;
        it.normal   = renderText(ren, fontMenu, s, white);
        it.selected = renderText(ren, fontMenu, s, warm);
        menu.push_back(it);
    }

    Texture signText = renderText(ren, fontSign, "Sign in / Sign up", SDL_Color{255, 230, 40, 255});

    int selectedIndex = 0;
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) running = false;

                if (e.key.keysym.sym == SDLK_UP) {
                    selectedIndex = (selectedIndex - 1 + (int)menu.size()) % (int)menu.size();
                } else if (e.key.keysym.sym == SDLK_DOWN) {
                    selectedIndex = (selectedIndex + 1) % (int)menu.size();
                } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) {
                    std::printf("[Title] Selected: %s\n", menu[selectedIndex].label.c_str());
                }
            }
        }

        SDL_RenderClear(ren);

        if (bg.t) {
            SDL_Rect dst{0,0,kDesignW,kDesignH};
            SDL_RenderCopy(ren, bg.t, nullptr, &dst);
        }

        int logoX = 120;
        int logoY = 70;
        if (logo.t) {
            int targetW = 520;
            int targetH = (int)((double)logo.h * (double)targetW / (double)std::max(1, logo.w));
            drawTexture(ren, logo, logoX, logoY, targetW, targetH);
        }

        int menuX = 140;
        int startY = 255;
        int gapY = 92;

        for (int i = 0; i < (int)menu.size(); ++i) {
            const bool isSel = (i == selectedIndex);

            const Texture& tex = isSel ? menu[i].selected : menu[i].normal;
            int x = menuX;
            int y = startY + i * gapY;

            if (isSel) {
                SDL_Rect bar{ menuX - 20, y + 18, 10, 44 };
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 160);
                SDL_RenderFillRect(ren, &bar);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
            }

            drawTextWithShadow(ren, tex, x, y, 3, 3);
        }

        if (signText.t) {
            int x = 135;
            int y = kDesignH - 95;

            SDL_SetTextureColorMod(signText.t, 0, 0, 0);
            SDL_Rect sh{ x + 3, y + 3, signText.w, signText.h };
            SDL_RenderCopy(ren, signText.t, nullptr, &sh);

            SDL_SetTextureColorMod(signText.t, 255, 230, 40);
            SDL_Rect dst{ x, y, signText.w, signText.h };
            SDL_RenderCopy(ren, signText.t, nullptr, &dst);

            SDL_SetTextureColorMod(signText.t, 255, 255, 255);
        }

        SDL_RenderPresent(ren);
    }

    destroy(signText);
    for (auto& it : menu) {
        destroy(it.normal);
        destroy(it.selected);
    }
    destroy(bg);
    destroy(logo);

    TTF_CloseFont(fontMenu);
    TTF_CloseFont(fontSign);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
