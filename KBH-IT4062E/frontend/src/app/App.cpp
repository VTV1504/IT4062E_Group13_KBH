#include "App.h"
#include <SDL_image.h>
#include <SDL_ttf.h>

#include "../ui/UiTheme.h"

bool App::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

    // PNG support is enough for now
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) return false;

    if (TTF_Init() != 0) return false;

    win = SDL_CreateWindow(
        "Keyboard Hero",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        UiTheme::DesignW,
        UiTheme::DesignH,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!win) return false;

    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) return false;

    // Use a fixed logical resolution for all screens/overlays
    SDL_RenderSetLogicalSize(ren, UiTheme::DesignW, UiTheme::DesignH);

    res.init(ren);
    return true;
}

void App::defer(std::function<void()> fn) {
    deferred.push_back(std::move(fn));
}

void App::processDeferred() {
    if (deferred.empty()) return;

    // Move queue out so deferred actions can schedule more deferred actions safely
    auto q = std::move(deferred);
    deferred.clear();

    for (auto& fn : q) {
        if (fn) fn();
    }
}

void App::run() {
    rt.change(RouteId::Title);

    Uint64 last = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();

    while (!quitRequested) {
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)((now - last) / freq);
        last = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quitRequested = true;
                break;
            }
            viewStack.handleEvent(e);
        }

        // Important: execute navigation/actions AFTER event processing to avoid use-after-free
        processDeferred();

        viewStack.update(dt);

        SDL_RenderClear(ren);
        viewStack.render(ren);
        SDL_RenderPresent(ren);
    }
}

void App::shutdown() {
    // Clean up resources safely
    res.shutdown();

    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}
