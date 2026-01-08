#include "TitleScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include <SDL_ttf.h>
#include <cmath>
#include <algorithm>

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

TitleScreen::TextTex TitleScreen::makeText(const std::string& s, int fontSize) {
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

void TitleScreen::destroyText(TextTex& t) {
    if (t.tex) SDL_DestroyTexture(t.tex);
    t.tex = nullptr;
    t.w = t.h = 0;
    t.text.clear();
}

void TitleScreen::drawTextShadow(SDL_Renderer* r, const TextTex& t, int x, int y, SDL_Color mainColor) {
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

void TitleScreen::windowToLogical(int wx, int wy, int& lx, int& ly) const {
    int outW = 0, outH = 0;
    SDL_GetRendererOutputSize(app->renderer(), &outW, &outH);
    if (outW <= 0) outW = UiTheme::DesignW;
    if (outH <= 0) outH = UiTheme::DesignH;

    lx = wx * UiTheme::DesignW / outW;
    ly = wy * UiTheme::DesignH / outH;
}

void TitleScreen::updateHoverFromMouse(int wx, int wy) {
    int mx, my;
    windowToLogical(wx, wy, mx, my);

    hoveredMenu = -1;
    hoveredSign = false;

    for (int i = 0; i < (int)menuRect.size(); ++i) {
        if (pointInRect(mx, my, menuRect[i])) {
            hoveredMenu = i;
            break;
        }
    }
    if (pointInRect(mx, my, signRect)) hoveredSign = true;
}

void TitleScreen::drawHoverButton(SDL_Renderer* r, const SDL_Rect& outer) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    // top/bottom border
    for (int x = outer.x; x < outer.x + outer.w; ++x) {
        float t = (outer.w <= 1) ? 0.0f : (float)(x - outer.x) / (float)(outer.w - 1);
        SDL_Color bc = hoverBorderAt(t);
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
        SDL_Color bc = hoverBorderAt(t);
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
        SDL_Color fc = hoverFillAt(t);
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

void TitleScreen::onEnter() {
    bg   = app->resources().texture(UiTheme::TitleBgPath);
    logo = app->resources().texture(UiTheme::TitleLogoPath);

    menuText.clear();
    menuRect.clear();

    for (auto& s : labels) {
        menuText.push_back(makeText(s, 64));
        menuRect.push_back(SDL_Rect{0,0,0,0});
    }
    signText = makeText("Sign in / Sign up", 58);

    // ====== TỌA ĐỘ THEO BẠN ======
    // Create Room: x=77, y=403
    // 4 nút cách nhau 15px theo chiều dọc
    const int menuX = 77;
    const int firstY = 403;
    const int gapY = btnH + 15;

    for (int i = 0; i < (int)labels.size(); ++i) {
        menuRect[i] = SDL_Rect{ menuX, firstY + i * gapY, btnW, btnH };
    }

    // Sign in: x=77, y=821
    // bạn đưa toạ độ là điểm đặt text, còn rect clickable mình set theo vùng button-like
    signRect = SDL_Rect{ 77, 821, 520, 80 };

    hoveredMenu = -1;
    hoveredSign = false;
}

void TitleScreen::onExit() {
    for (auto& t : menuText) destroyText(t);
    menuText.clear();
    menuRect.clear();
    destroyText(signText);

    bg = logo = nullptr;
}

void TitleScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        updateHoverFromMouse(e.motion.x, e.motion.y);
        return;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        updateHoverFromMouse(e.button.x, e.button.y);

        if (hoveredMenu != -1) {
            if (hoveredMenu == 0) app->router().change(RouteId::Lobby);
            else if (hoveredMenu == 1) app->router().push(RouteId::EnterRoomOverlay);
            else if (hoveredMenu == 2) { app->state().setPendingMode(GameMode::Training); app->router().change(RouteId::Game); }
            else if (hoveredMenu == 3) app->router().push(RouteId::LeaderboardOverlay);
            return;
        }

        if (hoveredSign) {
            if (app->session().isLoggedIn()) app->router().change(RouteId::Profile);
            else app->router().push(RouteId::SignInOverlay);
            return;
        }
    }

    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
        app->requestQuit();
    }
}

void TitleScreen::render(SDL_Renderer* r) {
    // background
    if (bg) {
        SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
        SDL_RenderCopy(r, bg, nullptr, &dst);
    } else {
        SDL_SetRenderDrawColor(r, 10, 10, 20, 255);
        SDL_Rect dst{0,0,UiTheme::DesignW,UiTheme::DesignH};
        SDL_RenderFillRect(r, &dst);
    }

    // ====== LOGO TỌA ĐỘ THEO BẠN ======
    // logo: x=133, y=-78, size 687x518
    if (logo) {
        SDL_Rect dst{ 133, -78, logoW, logoH };
        SDL_RenderCopy(r, logo, nullptr, &dst);
    }

    // menu
    for (int i = 0; i < (int)menuRect.size(); ++i) {
        const bool isHover = (i == hoveredMenu);

        if (isHover) drawHoverButton(r, menuRect[i]);

        // text inside button: left padding 20, vertically centered
        int tx = menuRect[i].x + 20;
        int ty = menuRect[i].y + (menuRect[i].h - menuText[i].h) / 2;

        drawTextShadow(r, menuText[i], tx, ty, UiTheme::White);
    }

    // sign in text
    {
        int sx = signRect.x;
        int sy = signRect.y;
        SDL_Color c = hoveredSign ? UiTheme::Warm : UiTheme::Yellow;
        drawTextShadow(r, signText, sx, sy, c);
    }
}
