#pragma once
#include "../core/View.h"

class ProfileScreen : public View {
public:
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;
};
