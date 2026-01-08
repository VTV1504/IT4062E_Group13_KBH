#pragma once
#include <SDL.h>

class App;

class View {
public:
    virtual ~View() = default;

    virtual void onEnter() {}
    virtual void onExit()  {}

    virtual void handleEvent(const SDL_Event& e) = 0;
    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* r) = 0;

    void setApp(App* a) { app = a; }
protected:
    App* app = nullptr;
};
