#include "ProfileScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include <SDL_ttf.h>
#include <cmath>
#include <algorithm>
#include <iostream>

static bool pointInRect(int x, int y, const SDL_Rect& r) {
    return (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
}

static Uint8 lerpU8(Uint8 a, Uint8 b, float t) {
    float v = a + (b - a) * t;
    if (v < 0) v = 0;
    if (v > 255) v = 255;
    return (Uint8)(v + 0.5f);
}

static SDL_Color lerpColor(SDL_Color a, SDL_Color b, float t) {
    SDL_Color c;
    c.r = lerpU8(a.r, b.r, t);
    c.g = lerpU8(a.g, b.g, t);
    c.b = lerpU8(a.b, b.b, t);
    c.a = lerpU8(a.a, b.a, t);
    return c;
}

static int circleDy(int radius, int dx) {
    double rr = (double)radius * (double)radius;
    double dd = (double)dx * (double)dx;
    double v = rr - dd;
    if (v <= 0.0) return 0;
    return (int)std::floor(std::sqrt(v));
}

// fill gradient: #f7cc6e -> rgba(...,0.25) @64.91% -> rgba(...,0)
static SDL_Color hoverFillAt(float t) {
    SDL_Color c0{247,204,110,255};
    SDL_Color c1{247,204,110,(Uint8)(0.25f * 255.0f)}; // ~64
    SDL_Color c2{247,204,110,0};

    const float mid = 0.6491f;
    if (t <= mid) {
        float u = (mid <= 0.0001f) ? 0.0f : (t / mid);
        return lerpColor(c0, c1, u);
    } else {
        float u = (t - mid) / (1.0f - mid);
        return lerpColor(c1, c2, u);
    }
}

// border gradient: #ffc038 -> rgba(...,0.25)
static SDL_Color hoverBorderAt(float t) {
    SDL_Color b0{255,192,56,255};
    SDL_Color b1{255,192,56,(Uint8)(0.25f * 255.0f)}; // ~64
    return lerpColor(b0, b1, t);
}

// Red hover fill for Sign Out button
static SDL_Color redHoverFillAt(float t) {
    SDL_Color c0{255,100,100,255};
    SDL_Color c1{255,100,100,(Uint8)(0.25f * 255.0f)};
    SDL_Color c2{255,100,100,0};

    const float mid = 0.6491f;
    if (t <= mid) {
        float u = (mid <= 0.0001f) ? 0.0f : (t / mid);
        return lerpColor(c0, c1, u);
    } else {
        float u = (t - mid) / (1.0f - mid);
        return lerpColor(c1, c2, u);
    }
}

// Red hover border for Sign Out button
static SDL_Color redHoverBorderAt(float t) {
    SDL_Color b0{255,80,80,255};
    SDL_Color b1{255,80,80,(Uint8)(0.25f * 255.0f)};
    return lerpColor(b0, b1, t);
}

ProfileScreen::TextTex ProfileScreen::makeText(const std::string& s, int fontSize) {
    TextTex out;
    out.text = s;

    TTF_Font* f = app->resources().font(UiTheme::MainFontPath, fontSize);
    if (!f) return out;

    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, s.c_str(), UiTheme::White);
    if (!surf) return out;

    out.w = surf->w;
    out.h = surf->h;
    out.tex = SDL_CreateTextureFromSurface(app->renderer(), surf);
    SDL_FreeSurface(surf);
    return out;
}

void ProfileScreen::destroyText(TextTex& t) {
    if (t.tex) SDL_DestroyTexture(t.tex);
    t.tex = nullptr;
    t.w = t.h = 0;
    t.text.clear();
}

void ProfileScreen::drawTextShadow(SDL_Renderer* r, const TextTex& t, int x, int y, SDL_Color mainColor) {
    if (!t.tex) return;

    SDL_SetTextureColorMod(t.tex, 0, 0, 0);

    // shadow 1: -2 -1, alpha 180
    SDL_SetTextureAlphaMod(t.tex, 180);
    SDL_Rect sh1{ x - 2, y - 1, t.w, t.h };
    SDL_RenderCopy(r, t.tex, nullptr, &sh1);

    // shadow 2: -1 +3, alpha 255
    SDL_SetTextureAlphaMod(t.tex, 255);
    SDL_Rect sh2{ x - 1, y + 3, t.w, t.h };
    SDL_RenderCopy(r, t.tex, nullptr, &sh2);

    // main
    SDL_SetTextureColorMod(t.tex, mainColor.r, mainColor.g, mainColor.b);
    SDL_SetTextureAlphaMod(t.tex, 255);
    SDL_Rect dst{ x, y, t.w, t.h };
    SDL_RenderCopy(r, t.tex, nullptr, &dst);

    SDL_SetTextureColorMod(t.tex, 255,255,255);
    SDL_SetTextureAlphaMod(t.tex, 255);
}

void ProfileScreen::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(app->renderer(), &outW, &outH);
    if (outW <= 0) outW = UiTheme::DesignW;
    if (outH <= 0) outH = UiTheme::DesignH;

    lx = wx * UiTheme::DesignW / outW;
    ly = wy * UiTheme::DesignH / outH;
}

void ProfileScreen::updateHoverFromMouse(int wx, int wy) {
    int mx, my;
    windowToLogical(wx, wy, mx, my);

    hoveredMenu = -1;
    hoveredSignOut = false;

    for (int i = 0; i < (int)menuRect.size(); ++i) {
        if (pointInRect(mx, my, menuRect[i])) {
            hoveredMenu = i;
            break;
        }
    }
    if (pointInRect(mx, my, signOutRect)) hoveredSignOut = true;
}

void ProfileScreen::drawHoverButton(SDL_Renderer* r, const SDL_Rect& outer, bool useRedColor) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // top/bottom border
    for (int x = outer.x; x < outer.x + outer.w; ++x) {
        float t = (outer.w <= 1) ? 0.0f : (float)(x - outer.x) / (float)(outer.w - 1);
        SDL_Color bc = useRedColor ? redHoverBorderAt(t) : hoverBorderAt(t);
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);

        int yTop0 = outer.y;
        int yTop1 = outer.y + btnBorder - 1;

        int yBot0 = outer.y + outer.h - btnBorder;
        int yBot1 = outer.y + outer.h - 1;

        if (x < outer.x + btnRadius) {
            int cx = outer.x + btnRadius;
            int dx = cx - x;
            int dy = circleDy(btnRadius, dx);
            int clipTop = outer.y + btnRadius - dy;
            int clipBot = outer.y + outer.h - btnRadius + dy - 1;

            int a0 = std::max(yTop0, clipTop);
            int a1 = std::min(yTop1, clipBot);
            if (a0 <= a1) SDL_RenderDrawLine(r, x, a0, x, a1);

            int b0 = std::max(yBot0, clipTop);
            int b1 = std::min(yBot1, clipBot);
            if (b0 <= b1) SDL_RenderDrawLine(r, x, b0, x, b1);
        } else {
            SDL_RenderDrawLine(r, x, yTop0, x, yTop1);
            SDL_RenderDrawLine(r, x, yBot0, x, yBot1);
        }
    }

    // left border
    for (int x = outer.x; x < outer.x + btnBorder; ++x) {
        float t = (outer.w <= 1) ? 0.0f : (float)(x - outer.x) / (float)(outer.w - 1);
        SDL_Color bc = useRedColor ? redHoverBorderAt(t) : hoverBorderAt(t);
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);

        int y0 = outer.y;
        int y1 = outer.y + outer.h - 1;

        if (x < outer.x + btnRadius) {
            int cx = outer.x + btnRadius;
            int dx = cx - x;
            int dy = circleDy(btnRadius, dx);
            y0 = outer.y + btnRadius - dy;
            y1 = outer.y + outer.h - btnRadius + dy - 1;
        }
        SDL_RenderDrawLine(r, x, y0, x, y1);
    }

    // fill
    SDL_Rect fill{
        outer.x + btnBorder,
        outer.y + btnBorder,
        outer.w - btnBorder,
        outer.h - btnBorder * 2
    };
    int fillRadius = std::max(0, btnRadius - btnBorder);

    for (int x = fill.x; x < fill.x + fill.w; ++x) {
        float t = (fill.w <= 1) ? 0.0f : (float)(x - fill.x) / (float)(fill.w - 1);
        SDL_Color fc = useRedColor ? redHoverFillAt(t) : hoverFillAt(t);
        SDL_SetRenderDrawColor(r, fc.r, fc.g, fc.b, fc.a);

        int y0 = fill.y;
        int y1 = fill.y + fill.h - 1;

        if (fillRadius > 0 && x < fill.x + fillRadius) {
            int cx = fill.x + fillRadius;
            int dx = cx - x;
            int dy = circleDy(fillRadius, dx);
            y0 = fill.y + fillRadius - dy;
            y1 = fill.y + fill.h - fillRadius + dy - 1;
        }
        SDL_RenderDrawLine(r, x, y0, x, y1);
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void ProfileScreen::onEnter() {
    std::cout << "[ProfileScreen] onEnter() start\n";
    std::cout << "[ProfileScreen] this = " << this << "\n";
    std::cout << "[ProfileScreen] app = " << app << "\n";
    
    std::cout << "[ProfileScreen] Loading bg texture...\n";
    bg = app->resources().texture(UiTheme::TitleBgPath);
    std::cout << "[ProfileScreen] bg loaded: " << (bg ? "OK" : "NULL") << "\n";
    
    std::cout << "[ProfileScreen] Loading logo texture...\n";
    logo = app->resources().texture(UiTheme::TitleLogoPath);
    std::cout << "[ProfileScreen] logo loaded: " << (logo ? "OK" : "NULL") << "\n";

    std::cout << "[ProfileScreen] About to clear vectors...\n";
    std::cout << "[ProfileScreen] menuText.size() before clear: " << menuText.size() << "\n";
    
    // Don't clear, just resize to 0
    menuText.resize(0);
    menuRect.resize(0);
    std::cout << "[ProfileScreen] Vectors resized to 0\n";

    // Only 2 menu items for ProfileScreen
    std::vector<std::string> labels {
        "Return to Title Screen",
        "Change Password"
    };
    
    std::cout << "[ProfileScreen] Created labels vector with " << labels.size() << " items\n";

    for (size_t i = 0; i < labels.size(); ++i) {
        std::cout << "[ProfileScreen] Creating text for item " << i << ": " << labels[i] << "\n";
        TextTex t = makeText(labels[i], 64);
        std::cout << "[ProfileScreen] Text created, pushing to menuText...\n";
        menuText.push_back(t);
        std::cout << "[ProfileScreen] Pushed to menuText, pushing rect...\n";
        menuRect.push_back(SDL_Rect{0,0,0,0});
        std::cout << "[ProfileScreen] Rect pushed\n";
    }
    
    std::cout << "[ProfileScreen] Creating signOutText...\n";
    signOutText = makeText("Sign Out", 58);
    std::cout << "[ProfileScreen] signOutText created\n";

    // Layout - same as TitleScreen
    const int menuX = 77;
    const int firstY = 403;
    const int gapY = btnH + 15;

    for (int i = 0; i < (int)menuText.size(); ++i) {
        menuRect[i] = SDL_Rect{ menuX, firstY + i * gapY, btnW, btnH };
    }

    // Sign out at same position as Sign In in TitleScreen: x=77, y=821
    signOutRect = SDL_Rect{ 77, 821, 520, 80 };

    hoveredMenu = -1;
    hoveredSignOut = false;
}

void ProfileScreen::onExit() {
    for (auto& t : menuText) destroyText(t);
    menuText.clear();
    menuRect.clear();
    destroyText(signOutText);

    bg = logo = nullptr;
}

void ProfileScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        updateHoverFromMouse(e.motion.x, e.motion.y);
        return;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        updateHoverFromMouse(e.button.x, e.button.y);

        if (hoveredMenu != -1) {
            // Return to Title Screen
            if (hoveredMenu == 0) {
                app->router().change(RouteId::Title);
            }
            // Change Password
            else if (hoveredMenu == 1) {
                app->router().push(RouteId::ChangePasswordOverlay);
            }
            return;
        }

        if (hoveredSignOut) {
            // Send sign_out to backend, then clear user state and return to title screen
            app->network().send_sign_out();
            app->state().clearUser();
            app->router().change(RouteId::Title);
            return;
        }
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        app->router().change(RouteId::Title);
    }
}

void ProfileScreen::render(SDL_Renderer* r) {
    // background
    if (bg) {
        SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
        SDL_RenderCopy(r, bg, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(r, 10, 10, 20, 255);
        SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
        SDL_RenderFillRect(r, &dst);
    }

    // logo: x=133, y=-78, size 687x518
    if (logo) {
        SDL_Rect dst{ 133, -78, logoW, logoH };
        SDL_RenderCopy(r, logo, nullptr, &dst);
    }

    // menu buttons
    for (int i = 0; i < (int)menuRect.size(); ++i) {
        const bool isHover = (i == hoveredMenu);

        if (isHover) drawHoverButton(r, menuRect[i], false);

        // text inside button: left padding 20, vertically centered
        int tx = menuRect[i].x + 20;
        int ty = menuRect[i].y + (menuRect[i].h - menuText[i].h) / 2;

        drawTextShadow(r, menuText[i], tx, ty, UiTheme::White);
    }

    // sign out button with red hover
    {
        if (hoveredSignOut) drawHoverButton(r, signOutRect, true);
        
        int sx = signOutRect.x + 20;
        int sy = signOutRect.y;
        SDL_Color c = hoveredSignOut ? SDL_Color{255,100,100,255} : UiTheme::Yellow;
        drawTextShadow(r, signOutText, sx, sy, c);
    }
}
