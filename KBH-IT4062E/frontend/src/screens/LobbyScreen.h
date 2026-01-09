#pragma once
#include "../core/View.h"
#include <string>

class LobbyScreen : public View {
public:
    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;

private:
    std::string selectedDifficulty = "easy";
};
