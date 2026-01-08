#pragma once
#include "../core/View.h"
#include "../state/Models.h"

class GameScreen : public View {
public:
    void onEnter() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    GameMode mode = GameMode::Training;
};
