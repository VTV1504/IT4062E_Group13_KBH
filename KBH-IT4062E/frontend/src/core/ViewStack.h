#pragma once
#include <memory>
#include <vector>
#include "View.h"

class App;

class ViewStack {
public:
    explicit ViewStack(App* app) : app(app) {}

    void clearAndPush(std::unique_ptr<View> v);
    void push(std::unique_ptr<View> v);
    void pop();
    void replaceTop(std::unique_ptr<View> v);

    View* top();
    View* belowTop();

    void handleEvent(const SDL_Event& e);
    void update(float dt);
    void render(SDL_Renderer* r);

private:
    App* app;
    std::vector<std::unique_ptr<View>> stack;
};
