#pragma once
#ifndef COMMON_H
#define COMMON_H

#include <array>
#include <cstdint>

#define FWD_DECL(x) using x = struct x
#define RGBA(x) x.r, x.g, x.b, x.a

constexpr char FONT_NAME[] = "res/Roboto.ttf";
constexpr int FONT_SIZE = 40;

constexpr int BOARD_SIZE = 9;
constexpr int SUBGRID_SIZE = 3;
constexpr int BOARD_DIM = 9;

using NumberGrid = std::array<std::array<uint8_t, BOARD_DIM>, BOARD_DIM>;

FWD_DECL(SDL_Window);
FWD_DECL(SDL_Renderer);
FWD_DECL(SDL_Texture);
FWD_DECL(TTF_Font);
FWD_DECL(Mix_Chunk);
FWD_DECL(SDL_Rect);
FWD_DECL(SDL_Color);
using SDL_Event = union SDL_Event;

struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;

  operator const SDL_Color &() const {
    return *reinterpret_cast<const SDL_Color *>(this);
  }
};

struct Rect {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;

  operator SDL_Rect *() {
    return reinterpret_cast<SDL_Rect *>(this);
  }

  operator const SDL_Rect *() const {
    return reinterpret_cast<const SDL_Rect *>(this);
  }
};

#endif //!COMMON_H
