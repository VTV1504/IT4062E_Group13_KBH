#pragma once
#include "../core/View.h"

class LeaderboardOverlay : public View {
public:
    void onEnter() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override;
    void render(SDL_Renderer* r) override;

private:
    bool showTraining = true;
    std::string statusMessage;
};
