#include "TextureManager.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

TextureManager::TextureManager(SDL_Renderer *renderer)
  : renderer(renderer) {
}

TextureManager::~TextureManager() {
  clearCache();
}

SDL_Texture *TextureManager::getTextTexture(std::string_view text, TTF_Font *font, Color color) {
  std::string key = std::string(text) + "_" + std::to_string(color.r) + std::to_string(color.g) + std::to_string(color.b) + std::to_string(color.a);

  if (textCache.find(key) == textCache.end()) {
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.data(), color);
    if (!surface) return nullptr;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    textCache[key] = texture;
  }
  return textCache[key];
}

void TextureManager::clearCache() {
  for (auto &pair : textCache) {
    if (pair.second) {
      SDL_DestroyTexture(pair.second);
    }
  }
  textCache.clear();
}

