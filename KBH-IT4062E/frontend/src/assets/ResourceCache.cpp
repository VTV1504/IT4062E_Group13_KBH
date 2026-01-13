#include "ResourceCache.h"
#include <SDL_image.h>
#include <iostream>

SDL_Texture* ResourceCache::texture(const std::string& path) {
    std::cout << "[ResourceCache] Requesting texture: " << path << "\n";
    
    if (!ren) {
        std::cerr << "[ResourceCache] Error: renderer is nullptr!\n";
        return nullptr;
    }
    
    auto it = textures.find(path);
    if (it != textures.end()) {
        std::cout << "[ResourceCache] Texture found in cache\n";
        return it->second;
    }

    std::cout << "[ResourceCache] Loading image from disk...\n";
    SDL_Surface* s = IMG_Load(path.c_str());
    if (!s) {
        std::cerr << "[ResourceCache] IMG_Load failed: " << IMG_GetError() << "\n";
        return nullptr;
    }

    std::cout << "[ResourceCache] Creating texture from surface...\n";
    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
    SDL_FreeSurface(s);

    if (!t) {
        std::cerr << "[ResourceCache] SDL_CreateTextureFromSurface failed: " << SDL_GetError() << "\n";
        return nullptr;
    }
    
    std::cout << "[ResourceCache] Texture created successfully\n";
    textures[path] = t;
    return t;
}

TTF_Font* ResourceCache::font(const std::string& path, int size) {
    std::string key = path + "#" + std::to_string(size);
    auto it = fonts.find(key);
    if (it != fonts.end()) return it->second;

    TTF_Font* f = TTF_OpenFont(path.c_str(), size);
    if (f) fonts[key] = f;
    return f;
}

void ResourceCache::shutdown() {
    for (auto& kv : textures) SDL_DestroyTexture(kv.second);
    textures.clear();
    for (auto& kv : fonts) TTF_CloseFont(kv.second);
    fonts.clear();
}
