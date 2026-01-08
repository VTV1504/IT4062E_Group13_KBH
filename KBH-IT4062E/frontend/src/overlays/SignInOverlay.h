#pragma once
#include "../core/View.h"
#include <string>

class SignInOverlay : public View {
public:
    void onEnter() override;
    void onExit() override;

    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    std::string username;
};
