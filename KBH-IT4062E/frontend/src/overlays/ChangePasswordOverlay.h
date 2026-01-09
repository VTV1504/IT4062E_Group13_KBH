#pragma once
#include "../core/View.h"
#include <string>

class ChangePasswordOverlay : public View {
public:
    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float) override;
    void render(SDL_Renderer* r) override;

private:
    std::string oldPassword;
    std::string newPassword;
    std::string confirm;
    std::string statusMessage;
    int activeField = 0;
    bool awaitingResponse = false;
};
