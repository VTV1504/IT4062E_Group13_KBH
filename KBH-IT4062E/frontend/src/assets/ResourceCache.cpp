#include "ResourceCache.h"
#include <SDL_image.h>

SDL_Texture* ResourceCache::texture(const std::string& path) {
    auto it = textures.find(path);
    if (it != textures.end()) return it->second;

    SDL_Surface* s = IMG_Load(path.c_str());
    if (!s) return nullptr;

    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
    SDL_FreeSurface(s);

    if (t) textures[path] = t;
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
