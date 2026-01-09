#pragma once
#include "../core/View.h"
#include "../state/Models.h"
#include <string>
#include <SDL.h>

class GameScreen : public View {
public:
    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override;
    void render(SDL_Renderer* r) override;

private:
    GameMode mode = GameMode::Training;
    std::string targetText;
    std::string typedText;
    Uint32 startTicks = 0;
    int durationSeconds = 0;
    std::string helperMessage;
};
