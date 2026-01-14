#pragma once
#include "../core/View.h"
#include <string>
#include <iostream>

class ChangePasswordOverlay : public View {
public:
    ChangePasswordOverlay() 
        : currentPassword_(""),
          newPassword_(""),
          confirmPassword_(""),
          errorMessage_(""),
          activeField_(0), 
          exitButtonTexture_(nullptr) {
        std::cout << "[ChangePasswordOverlay] Constructor called\n";
    }
    
    void onEnter() override;
    void onExit() override;
    void onResume() override;

    void handleEvent(const SDL_Event& e) override;
    void update(float) override {}
    void render(SDL_Renderer* r) override;
    
    void setError(const std::string& error) { errorMessage_ = error; }

private:
    std::string currentPassword_;
    std::string newPassword_;
    std::string confirmPassword_;
    std::string errorMessage_;
    int activeField_ = 0; // 0=current, 1=new, 2=confirm
    
    SDL_Rect currentPasswordField_{};
    SDL_Rect newPasswordField_{};
    SDL_Rect confirmPasswordField_{};
    SDL_Rect changeButton_{};
    SDL_Rect exitButton_{};
    
    SDL_Texture* exitButtonTexture_ = nullptr;
    
    void handleChange();
    void handleExit();
    bool isPointInRect(int x, int y, const SDL_Rect& rect);
};
