#pragma once
#include "../core/View.h"
#include <string>

class SignInOverlay : public View {
public:
    void onEnter() override;
    void onExit() override;
    void onResume() override;

    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;
    
    void setError(const std::string& error) { errorMessage_ = error; }

private:
    std::string username_;
    std::string password_;
    std::string errorMessage_;
    bool usernameActive_ = true;
    
    SDL_Rect usernameField_{};
    SDL_Rect passwordField_{};
    SDL_Rect signInButton_{};
    SDL_Rect createAccountButton_{};
    SDL_Rect exitButton_{};
    
    SDL_Texture* exitButtonTexture_ = nullptr;
    
    void handleSignIn();
    void handleCreateAccount();
    void handleExit();
    bool isPointInRect(int x, int y, const SDL_Rect& rect);
};

