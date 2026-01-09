#pragma once
#include "../core/View.h"
#include <string>

class EnterRoomOverlay : public View {
public:
    void onEnter() override;
    void onExit() override;

    void handleEvent(const SDL_Event& e) override;
    void update(float) override;
    void render(SDL_Renderer* r) override;

private:
    std::string code;
    std::string statusMessage;
    std::string mode = "arena";
    bool matchmaking = false;
    bool awaitingResponse = false;
};
