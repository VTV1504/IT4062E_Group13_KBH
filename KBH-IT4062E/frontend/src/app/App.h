#pragma once
#include <SDL.h>
#include <functional>
#include <vector>

#include "../core/ViewStack.h"
#include "../core/Router.h"
#include "../core/NetworkClient.h"
#include "../assets/ResourceCache.h"
#include "../state/Session.h"
#include "../state/AppState.h"

class App {
public:
    bool init();
    void run();
    void shutdown();

    void requestQuit() { quitRequested = true; }
    bool shouldQuit() const { return quitRequested; }

    SDL_Renderer* renderer() { return ren; }
    ResourceCache& resources() { return res; }
    Session& session() { return sess; }
    AppState& state() { return st; }
    ViewStack& views() { return viewStack; }
    Router& router() { return rt; }
    NetworkClient& network() { return net; }

    // Defer navigation/actions to run AFTER event processing (prevents use-after-free)
    void defer(std::function<void()> fn);
    void processDeferred();

private:
    void handleServerMessage(const Json::Value& payload);

    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;

    bool quitRequested = false;

    ResourceCache res;
    Session sess;
    AppState st;
    NetworkClient net;

    ViewStack viewStack{this};
    Router rt{this};

    std::vector<std::function<void()>> deferred;
};
