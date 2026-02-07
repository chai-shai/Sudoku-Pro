#pragma once
#ifndef SUDOKU_H
#define SUDOKU_H

#include "Common.h"

#include <chrono>
#include <atomic>
#include <thread>
#include <memory>

class TextureManager;

class Sudoku {
  public:
    Sudoku();
    ~Sudoku();
    int run();
  private:
    bool init();
    bool loadMedia();
    void handleEvents();
    void render();
    void renderUI();
    void update();
    void initializeNewGame(bool generateNew);
    void generatePuzzle();
    void calculateLayout();
    void handleMouseEvents();
    void handleKeyboardEvents(int key);
    void handleArrowKeys(int key);
    void handleNumericKeys(int key);
    void handleDifficultyKeys(int key);
    void highlightGrid();
    void showGridLines();
    Color getCellColor(int row, int col);
    void showNumbers();
    void showHud();
    void showPauseScreen();
    void showWinScreen();
    void showLoadingScreen();
    void showNumberStats();
  private:
    void generatePuzzleTask();
  private:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    TTF_Font *font = nullptr;
    Mix_Chunk *moveSound = nullptr;
    Mix_Chunk *inputSound = nullptr;
    Mix_Chunk *winSound = nullptr;
    Mix_Chunk *mistakeSound = nullptr;

    std::unique_ptr<TextureManager> textureManager;

    bool isRunning = true;
    bool isPaused = false;
    bool isPuzzleSolved = false;
    bool isShowingMenu = true;
    bool isMuted = false;

    int fontSizeMode = 1;
    int maxMistakes = 3;

    int selectedRow = -1;
    int selectedCol = -1;

    NumberGrid board{};
    NumberGrid fixedBoard{};
    NumberGrid solvedBoard{};

    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point pauseStartTime{};
    std::chrono::seconds totalPausedDuration{};
    std::chrono::seconds finalElapsedDuration{};

    int mistakeCount = 0;
    int targetClues = 45;

    int windowWidth = 800;
    int windowHeight = 800;
    int boardSize = 0;
    int cellSize = 0;
    int offsetX = 0;
    int offsetY = 0;
  private:
    std::atomic_bool isGenerating = false;
    std::thread generationThread;
};

#endif //!SUDOKU_H
