#include "GameScreen.h"
#include "../app/App.h"
#include "../ui/UiTheme.h"
#include "../ui/TextDraw.h"
#include <iostream>
#include <sstream>
#include <algorithm>

void GameScreen::onEnter() {
    std::cout << "[GameScreen] onEnter()\n";
    
    try {
        // Load background
        bgTexture = app->resources().texture(UiTheme::GameScreenBackground);
        if (!bgTexture) {
            std::cerr << "[GameScreen] Failed to load background\n";
        }
        
        // Load knight textures
        for (int i = 0; i < 8; ++i) {
            knightTextures[i] = app->resources().texture(UiTheme::LobbyKnightPaths[i]);
            if (!knightTextures[i]) {
                std::cerr << "[GameScreen] Failed to load knight " << i << "\n";
            }
        }
        
        std::cout << "[GameScreen] About to load game state\n";
        // Load game state from server (AppState)
        loadGameStateFromServer();
        
        std::cout << "[GameScreen] About to set local game start\n";
        // Store local game start time for timestamp conversion
        local_game_start = SDL_GetTicks64();
        
        std::cout << "[GameScreen] About to split paragraph\n";
        splitParagraphIntoWords();
        
        std::cout << "[GameScreen] About to start text input\n";
        // Only start text input if not already started
        if (!SDL_IsTextInputActive()) {
            SDL_StartTextInput();
        }
        
        std::cout << "[GameScreen] onEnter() completed successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "[GameScreen] Exception in onEnter(): " << e.what() << "\n";
        throw;
    }
}

void GameScreen::onExit() {
    std::cout << "[GameScreen] onExit()\n";
    bgTexture = nullptr;
    for (int i = 0; i < 8; ++i) {
        knightTextures[i] = nullptr;
    }
    SDL_StopTextInput();
}

void GameScreen::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_BACKSPACE && !playerInput.empty()) {
            playerInput.pop_back();
            
            // Track backspace event with server timestamp
            CharEvent ce;
            ce.ch = '\b';
            ce.backspace = true;
            // Convert local time to server time
            int64_t local_elapsed = SDL_GetTicks64() - local_game_start;
            ce.time_ms = game_start_time + local_elapsed;
            currentWordCharEvents.push_back(ce);
        }
        else if (e.key.keysym.sym == SDLK_SPACE) {
            // Check if current word is complete
            if (currentWordIndex < (int)paragraphWords.size()) {
                const std::string& targetWord = paragraphWords[currentWordIndex];
                if (playerInput == targetWord) {
                    // Correct! Send to server and move to next word
                    sendInputToServer();
                    currentWordIndex++;
                    playerInput.clear();
                    currentWordCharEvents.clear();
                    scrollIfNeeded();
                    
                    std::cout << "[GameScreen] Completed word " << currentWordIndex 
                              << "/" << paragraphWords.size() << "\n";
                } else {
                    // Wrong word, don't advance
                    std::cout << "[GameScreen] Wrong word: '" << playerInput 
                              << "' != '" << targetWord << "'\n";
                }
            }
        }
        else if (e.key.keysym.sym == SDLK_ESCAPE) {
            // Back to lobby
            app->router().pop();
        }
    }
    
    if (e.type == SDL_TEXTINPUT) {
        char ch = e.text.text[0];
        if (ch != ' ' && ch != '\n' && ch != '\r') {
            playerInput += ch;
            
            // Track character event
            CharEvent ce;
            ce.ch = ch;
            ce.backspace = false;
            // Convert local time to server time
            int64_t local_elapsed = SDL_GetTicks64() - local_game_start;
            ce.time_ms = game_start_time + local_elapsed;
            currentWordCharEvents.push_back(ce);
        }
    }
}

void GameScreen::update(float dt) {
    (void)dt;
    
    // Update player progress from game_state events
    if (app->state().hasGameState()) {
        const auto& gs = app->state().getGameState();
        
        for (int i = 0; i < 8; ++i) {
            if (gs.players[i].occupied) {
                players[i].progress = (float)gs.players[i].progress;
            }
        }
        
        // Check if game ended
        if (gs.ended) {
            std::cout << "[GameScreen] Game ended detected in game_state\n";
            // App.cpp will handle navigation
        }
    } else {
        // Fallback: calculate progress from currentWordIndex for self
        if (self_slot_idx >= 0 && self_slot_idx < 8) {
            players[self_slot_idx].progress = (float)currentWordIndex / (float)paragraphWords.size();
        }
    }
}

void GameScreen::render(SDL_Renderer* r) {
    // Background
    if (bgTexture) {
        SDL_Rect bgRect{0, 0, 1536, 1024};
        SDL_RenderCopy(r, bgTexture, nullptr, &bgRect);
    } else {
        SDL_SetRenderDrawColor(r, 20, 20, 30, 255);
        SDL_RenderClear(r);
    }
    
    renderKnights(r);
    renderInputZone(r);
}

void GameScreen::renderKnights(SDL_Renderer* r) {
    const int knightSize = 180;
    
    for (int i = 0; i < 8; ++i) {
        if (!players[i].active) continue;
        
        int y = getKnightY(i);
        int x = getKnightX(players[i].progress);
        
        // Render knight
        if (knightTextures[players[i].knight_idx]) {
            SDL_Rect knightRect{x, y, knightSize, knightSize};
            SDL_RenderCopy(r, knightTextures[players[i].knight_idx], nullptr, &knightRect);
        }
        
        // Render name above knight
        TTF_Font* nameFont = app->resources().font(UiTheme::MainFontPath, 22);
        if (nameFont) {
            SDL_Color nameColor = (i == self_slot_idx) ? UiTheme::Yellow : UiTheme::White;
            int nameX = x + knightSize / 2 - 40; // Center approximately
            int nameY = y - 25;
            drawText(r, nameFont, players[i].name, nameX, nameY, nameColor);
        }
    }
}

void GameScreen::renderInputZone(SDL_Renderer* r) {
    // InputZone: 1350x166 at (93, 848), color #382000 opacity 49%
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0x38, 0x20, 0x00, (Uint8)(0.49 * 255));
    SDL_Rect inputZone{93, 848, 1350, 166};
    SDL_RenderFillRect(r, &inputZone);
    
    renderParagraphBox(r);
    renderTextField(r);
}

void GameScreen::renderParagraphBox(SDL_Renderer* r) {
    // ParagraphBox: 1270x77 at (135, 865), font size 30
    TTF_Font* paragraphFont = app->resources().font(UiTheme::MainFontPath, 30);
    if (!paragraphFont) return;
    
    int x = 135;
    int y = 865;
    int maxWidth = 1270;
    int lineHeight = 40;
    
    // Build colored segments for visible words (2 lines)
    std::vector<ColoredSegment> segments;
    int numWordsShown = 0;
    buildVisibleWordsWithColors(segments, numWordsShown);
    
    // Render segments across 2 lines
    int currentX = x;
    int currentY = y;
    int lineNum = 0;
    
    for (const auto& seg : segments) {
        if (seg.text.empty()) continue;
        
        // Check if this segment fits on current line
        int w, h;
        TTF_SizeText(paragraphFont, seg.text.c_str(), &w, &h);
        
        if (currentX + w > x + maxWidth && currentX > x) {
            // Move to next line
            lineNum++;
            currentX = x;
            currentY += lineHeight;
            
            if (lineNum >= 2) break; // Only 2 lines
        }
        
        // Render this segment
        drawText(r, paragraphFont, seg.text, currentX, currentY, seg.color);
        currentX += w;
    }
}

void GameScreen::renderTextField(SDL_Renderer* r) {
    // TextField: 1270x38, white background, font size 22
    int x = 135;
    int y = 865 + 77 + 10; // Below paragraph box with 10px gap
    int width = 1270;
    int height = 38;
    
    // White background
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_Rect textFieldRect{x, y, width, height};
    SDL_RenderFillRect(r, &textFieldRect);
    
    // Render input text
    TTF_Font* inputFont = app->resources().font(UiTheme::MainFontPath, 22);
    if (inputFont && !playerInput.empty()) {
        drawText(r, inputFont, playerInput, x + 5, y + 8, {0, 0, 0, 255});
    }
}

void GameScreen::splitParagraphIntoWords() {
    std::cout << "[GameScreen] splitParagraphIntoWords() called\n";
    
    try {
        paragraphWords.clear();
        paragraphWords.reserve(100); // Pre-allocate to avoid reallocs
        
        if (paragraph.empty()) {
            std::cerr << "[GameScreen] Warning: paragraph is empty!\n";
            return;
        }
        
        std::cout << "[GameScreen] Paragraph length: " << paragraph.length() << "\n";
        std::cout << "[GameScreen] First 50 chars: " << paragraph.substr(0, std::min<size_t>(50, paragraph.length())) << "\n";
        
        std::istringstream iss(paragraph);
        std::string word;
        int count = 0;
        while (iss >> word) {
            paragraphWords.push_back(word);
            count++;
            if (count % 10 == 0) {
                std::cout << "[GameScreen] Split " << count << " words so far...\n";
            }
        }
        std::cout << "[GameScreen] Split paragraph into " << paragraphWords.size() << " words\n";
    } catch (const std::exception& e) {
        std::cerr << "[GameScreen] Exception in splitParagraphIntoWords(): " << e.what() << "\n";
        throw;
    }
}

std::vector<GameScreen::ColoredSegment> GameScreen::getWordSegments(int wordIndex) const {
    std::vector<ColoredSegment> segments;
    
    if (wordIndex >= (int)paragraphWords.size()) {
        return segments;
    }
    
    const std::string& targetWord = paragraphWords[wordIndex];
    
    if (wordIndex < currentWordIndex) {
        // Completed word - all green
        segments.push_back({targetWord, {0, 255, 0, 255}});
    }
    else if (wordIndex == currentWordIndex) {
        // Current word being typed - compare with playerInput
        size_t matchLen = 0;
        size_t errorPos = std::string::npos;
        
        for (size_t i = 0; i < playerInput.length(); ++i) {
            if (i < targetWord.length() && playerInput[i] == targetWord[i]) {
                matchLen = i + 1;
            } else {
                errorPos = i;
                break;
            }
        }
        
        // Green part (correct)
        if (matchLen > 0) {
            segments.push_back({targetWord.substr(0, matchLen), {0, 255, 0, 255}});
        }
        
        // Red part (first error)
        if (errorPos != std::string::npos && errorPos < targetWord.length()) {
            segments.push_back({targetWord.substr(errorPos, 1), {255, 0, 0, 255}});
            
            // White part (remaining after error)
            if (errorPos + 1 < targetWord.length()) {
                segments.push_back({targetWord.substr(errorPos + 1), UiTheme::White});
            }
        } else if (matchLen < targetWord.length()) {
            // White part (not yet typed)
            segments.push_back({targetWord.substr(matchLen), UiTheme::White});
        }
    }
    else {
        // Future word - all white
        segments.push_back({targetWord, UiTheme::White});
    }
    
    return segments;
}

void GameScreen::buildVisibleWordsWithColors(std::vector<ColoredSegment>& segments, int& numWordsShown) {
    segments.clear();
    numWordsShown = 0;
    
    TTF_Font* font = app->resources().font(UiTheme::MainFontPath, 30);
    if (!font) return;
    
    int maxWidth = 1270;
    int currentLineWidth = 0;
    int lineCount = 0;
    
    // Start from firstVisibleWordIndex
    for (int i = firstVisibleWordIndex; i < (int)paragraphWords.size(); ++i) {
        auto wordSegs = getWordSegments(i);
        
        // Calculate total width of this word
        int wordWidth = 0;
        for (const auto& seg : wordSegs) {
            int w, h;
            TTF_SizeText(font, seg.text.c_str(), &w, &h);
            wordWidth += w;
        }
        
        // Add space before word (except first word in line)
        int spaceWidth = 0;
        if (currentLineWidth > 0) {
            int w, h;
            TTF_SizeText(font, " ", &w, &h);
            spaceWidth = w;
        }
        
        // Check if word fits on current line
        if (currentLineWidth + spaceWidth + wordWidth > maxWidth && currentLineWidth > 0) {
            // Move to next line
            lineCount++;
            currentLineWidth = 0;
            spaceWidth = 0;
            
            if (lineCount >= 2) break; // Only 2 lines
        }
        
        // Add space if needed
        if (spaceWidth > 0) {
            segments.push_back({" ", UiTheme::White});
            currentLineWidth += spaceWidth;
        }
        
        // Add word segments
        for (const auto& seg : wordSegs) {
            segments.push_back(seg);
        }
        currentLineWidth += wordWidth;
        numWordsShown++;
    }
}

void GameScreen::scrollIfNeeded() {
    // Only scroll if current word is not in the visible 2-line window
    // First, check if current word is visible with current firstVisibleWordIndex
    TTF_Font* font = app->resources().font(UiTheme::MainFontPath, 30);
    if (!font) return;
    
    int maxWidth = 1270;
    int currentLineWidth = 0;
    int lineCount = 0;
    bool currentWordVisible = false;
    
    // Simulate rendering to see if currentWordIndex is visible
    for (int i = firstVisibleWordIndex; i < (int)paragraphWords.size(); ++i) {
        if (i == currentWordIndex) {
            currentWordVisible = true;
            break;
        }
        
        // Calculate word width
        int wordWidth = 0;
        std::string word = paragraphWords[i];
        int w, h;
        TTF_SizeText(font, word.c_str(), &w, &h);
        wordWidth = w;
        
        // Add space
        int spaceWidth = 0;
        if (currentLineWidth > 0) {
            TTF_SizeText(font, " ", &w, &h);
            spaceWidth = w;
        }
        
        // Check line break
        if (currentLineWidth + spaceWidth + wordWidth > maxWidth && currentLineWidth > 0) {
            lineCount++;
            currentLineWidth = 0;
            spaceWidth = 0;
            
            if (lineCount >= 2) break; // Only 2 lines visible
        }
        
        currentLineWidth += spaceWidth + wordWidth;
    }
    
    // If current word is not visible, scroll forward
    if (!currentWordVisible && currentWordIndex > firstVisibleWordIndex) {
        // Move firstVisibleWordIndex forward until currentWordIndex fits in the window
        firstVisibleWordIndex++;
        
        // Recursively check again (with limit to prevent infinite loop)
        static int recursionDepth = 0;
        if (recursionDepth < 10) {
            recursionDepth++;
            scrollIfNeeded();
            recursionDepth--;
        }
    }
}

void GameScreen::loadGameStateFromServer() {
    // Load from AppState (game_init event should have set this)
    if (app->state().hasGameInit()) {
        const auto& gi = app->state().getGameInit();
        
        // Load paragraph and words
        paragraph = gi.paragraph;
        game_start_time = gi.server_start_ms;
        game_duration_ms = gi.duration_ms;
        
        std::cout << "[GameScreen] Loaded from game_init:\n";
        std::cout << "  Paragraph: " << paragraph.substr(0, 60) << "...\n";
        std::cout << "  Duration: " << game_duration_ms << "ms\n";
        std::cout << "  Players: " << gi.players.size() << "\n";
        
        // Load players from game_init
        for (const auto& p : gi.players) {
            if (p.slot_idx >= 0 && p.slot_idx < 8) {
                players[p.slot_idx].active = true;
                players[p.slot_idx].name = p.display_name;
                players[p.slot_idx].progress = 0.0f;
                
                // Get knight_idx from room state
                if (app->state().hasRoom()) {
                    const auto& room = app->state().getRoomState();
                    players[p.slot_idx].knight_idx = room.slots[p.slot_idx].knight_idx;
                    
                    if (p.client_id == room.self_client_id) {
                        self_slot_idx = p.slot_idx;
                        std::cout << "  Self slot: " << self_slot_idx << "\n";
                    }
                }
            }
        }
    } else {
        // Fallback: test data
        std::cout << "[GameScreen] No game_init, using test data\n";
        paragraph = "The quick brown fox jumps over the lazy dog";
        players[0].active = true;
        players[0].name = "Player 1";
        players[0].knight_idx = 0;
        players[0].progress = 0.0f;
        self_slot_idx = 0;
    }
}

void GameScreen::sendInputToServer() {
    // Send completed word to server via input message
    // Format: {"type": "input", "room_id": "...", "word_idx": 5, "char_events": [...]}
    
    if (!app->state().hasGameInit()) {
        std::cout << "[GameScreen] No game_init, cannot send input\n";
        return;
    }
    
    const auto& gi = app->state().getGameInit();
    
    // Build char_events JSON array
    Json::Value charEventsArray(Json::arrayValue);
    for (const auto& ce : currentWordCharEvents) {
        Json::Value event;
        if (ce.backspace) {
            event["type"] = "backspace";
        } else {
            event["type"] = "char";
            event["char"] = std::string(1, ce.ch);
        }
        event["time_ms"] = (Json::Int64)ce.time_ms;
        charEventsArray.append(event);
    }
    
    std::cout << "[GameScreen] Sending word " << currentWordIndex 
              << " with " << currentWordCharEvents.size() << " char events\n";
    
    app->network().send_input(gi.room_id, currentWordIndex, charEventsArray);
}

int GameScreen::getKnightY(int slotIndex) const {
    // Linear interpolation from slot 0 at y=279 to slot 7 at y=590
    const int startY = 279;
    const int endY = 590;
    return startY + (endY - startY) * slotIndex / 7;
}

int GameScreen::getKnightX(float progress) const {
    const int startX = 61;
    const int endX = 1331;
    return startX + (int)((endX - startX) * progress);
}
