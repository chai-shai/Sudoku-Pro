#pragma once
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "Common.h"
#include <unordered_map>
#include <string>

class TextureManager {
  public:
    TextureManager(SDL_Renderer *renderer);
    ~TextureManager();
    SDL_Texture *getTextTexture(std::string_view text, TTF_Font *font, Color color);
    void clearCache();
  private:
    SDL_Renderer *renderer;
    std::unordered_map<std::string, SDL_Texture *> textCache;
};

#endif //!TEXTURE_MANAGER_H
