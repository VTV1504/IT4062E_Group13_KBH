#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <unordered_map>
#include <string>

class ResourceCache {
public:
    void init(SDL_Renderer* r) { ren = r; }

    SDL_Texture* texture(const std::string& path);
    TTF_Font* font(const std::string& path, int size);

    void shutdown();

private:
    SDL_Renderer* ren = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures;
    std::unordered_map<std::string, TTF_Font*> fonts;
};
