#pragma once
#include "../core/View.h"
#include <string>

class CreateAccountOverlay : public View {
public:
    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override;
    void render(SDL_Renderer* r) override;

private:
    std::string username;
    std::string password;
    std::string confirm;
    std::string statusMessage;
    int activeField = 0;
    bool awaitingResponse = false;
};
