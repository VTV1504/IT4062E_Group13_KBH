#pragma once
#include "../core/View.h"
#include <SDL_ttf.h>
#include <string>
#include <vector>

class GameScreen : public View {
public:
    void onEnter() override;
    void onExit() override;
    void handleEvent(const SDL_Event& e) override;
    void update(float dt) override;
    void render(SDL_Renderer* r) override;

private:
    // Assets
    SDL_Texture* bgTexture = nullptr;
    SDL_Texture* knightTextures[8] = {nullptr};
    
    // Game state from server
    struct PlayerState {
        bool active = false;
        std::string name;
        float progress = 0.0f; // 0.0 to 1.0
        int knight_idx = 0;
    };
    
    PlayerState players[8];
    std::string paragraph;
    int self_slot_idx = -1; // Which slot is this client
    
    // Input state
    std::string playerInput;
    std::vector<std::string> paragraphWords; // Split paragraph into words
    int currentWordIndex = 0; // Which word we're typing
    int firstVisibleWordIndex = 0; // First word shown in 2-line window (for scrolling)
    
    // Char events tracking for sending to server
    struct CharEvent {
        char ch;
        int64_t time_ms;
        bool backspace;
    };
    std::vector<CharEvent> currentWordCharEvents;
    
    // Game timing
    int64_t game_start_time = 0;  // Server time when game started
    int64_t local_game_start = 0; // Local SDL_GetTicks64() when game started
    int game_duration_ms = 50000;
    
    // Text rendering helpers
    struct ColoredSegment {
        std::string text;
        SDL_Color color;
    };
    
    void renderKnights(SDL_Renderer* r);
    void renderInputZone(SDL_Renderer* r);
    void renderParagraphBox(SDL_Renderer* r);
    void renderTextField(SDL_Renderer* r);
    
    // Text processing
    void splitParagraphIntoWords();
    std::vector<ColoredSegment> getWordSegments(int wordIndex) const;
    void buildVisibleWordsWithColors(std::vector<ColoredSegment>& segments, int& numWordsShown);
    void scrollIfNeeded();
    
    // Server integration
    void loadGameStateFromServer();
    void sendInputToServer();
    
    // Position helpers
    int getKnightY(int slotIndex) const;
    int getKnightX(float progress) const;
};
