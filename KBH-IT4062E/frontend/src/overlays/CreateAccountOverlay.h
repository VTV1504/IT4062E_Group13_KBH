#pragma once
#include "../core/View.h"
#include <string>
#include <iostream>

class CreateAccountOverlay : public View {
public:
    CreateAccountOverlay() 
        : activeField_(0), exitButtonTexture_(nullptr) {
        std::cout << "[CreateAccountOverlay] Constructor called\n";
    }
    
    void onEnter() override;
    void onExit() override;

    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;
    
    void setError(const std::string& error) { errorMessage_ = error; }

private:
    std::string username_;
    std::string password_;
    std::string confirmPassword_;
    std::string errorMessage_;
    int activeField_ = 0; // 0=username, 1=password, 2=confirmPassword
    
    SDL_Rect usernameField_{};
    SDL_Rect passwordField_{};
    SDL_Rect confirmPasswordField_{};
    SDL_Rect signUpButton_{};
    SDL_Rect exitButton_{};
    
    SDL_Texture* exitButtonTexture_ = nullptr;
    
    void handleSignUp();
    void handleExit();
    bool isPointInRect(int x, int y, const SDL_Rect& rect);
};
