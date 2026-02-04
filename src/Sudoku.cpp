#include "Sudoku.h"
#include "Solver.h"
#include "TextureManager.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

#include <iostream>
#include <random>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

Sudoku::Sudoku()
  : startTime(std::chrono::steady_clock::now()) {
  calculateLayout();
}

Sudoku::~Sudoku() {
  if (generationThread.joinable()) generationThread.join();

  Mix_FreeChunk(moveSound);
  Mix_FreeChunk(inputSound);
  Mix_FreeChunk(winSound);
  Mix_FreeChunk(mistakeSound);
  
  TTF_CloseFont(font);
  font = nullptr;

  SDL_DestroyRenderer(renderer);
  renderer = nullptr;

  SDL_DestroyWindow(window);
  window = nullptr;

  Mix_CloseAudio();
  TTF_Quit();
  SDL_Quit();
}

bool Sudoku::init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL couldn't initialize! SDL Error: " << SDL_GetError() << std::endl;
    return false;
  }

  if (TTF_Init() == -1) {
    std::cerr << "SDL_ttf couldn't initialize: SDL_ttf Error: " << TTF_GetError() << std::endl;
    return false;
  }

  constexpr int flags = MIX_INIT_OGG;
  if ((Mix_Init(flags) & flags) != flags) {
    std::cerr << "SDL_Mixer OGG Init Error: " << Mix_GetError() << std::endl;
  }

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    std::cerr << "SDL_mixer error: " << Mix_GetError() << std::endl;
  }

  if (!loadMedia()) return false;

  window = SDL_CreateWindow("sudoku",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    windowWidth, windowHeight,
    SDL_WINDOW_RESIZABLE
  );

  if (!window) {
    std::cerr << "Window couldn't be created! SDL Error: " << SDL_GetError() << std::endl;
    return false;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    std::cerr << "Renderer couldn't be created! SDL Error: " << SDL_GetError() << std::endl;
    return false;
  }

  font = TTF_OpenFont(FONT_NAME, FONT_SIZE);
  if (!font) {
    std::cerr << "Failed to load font! SDL_ttf Error: " << TTF_GetError() << std::endl;
    return false;
  }

  textureManager = std::make_unique<TextureManager>(renderer);
 
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer2_Init(renderer);
  return true;
}

bool Sudoku::loadMedia() {
  moveSound = Mix_LoadWAV("res/move.ogg");
  inputSound = Mix_LoadWAV("res/input.ogg");
  winSound = Mix_LoadWAV("res/win.ogg");
  mistakeSound = Mix_LoadWAV("res/mistake.ogg");

  if (!moveSound || !inputSound || !winSound || !mistakeSound) {
    std::cerr << "Failed to load sound effects! Mix_Error: " << Mix_GetError() << std::endl;
  }

  return true;
}

void Sudoku::handleEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (event.type == SDL_QUIT) {
      isRunning = false;
      return;
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
      if (event.type == SDL_MOUSEBUTTONDOWN) handleMouseEvents();
      if (event.type == SDL_KEYDOWN) handleKeyboardEvents(event.key.keysym.sym);
    }

    if (event.type == SDL_WINDOWEVENT) {
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
         windowWidth = event.window.data1;
        windowHeight = event.window.data2;
        textureManager->clearCache();
        calculateLayout();
      }
    }

    if (event.type == SDL_MOUSEBUTTONDOWN) {
      handleMouseEvents();
    } else if (event.type == SDL_KEYDOWN) {
      handleKeyboardEvents(event.key.keysym.sym);
    }
  }
}

void Sudoku::render() {
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_RenderClear(renderer);

  if (isShowingMenu) {
    SDL_SetRenderDrawColor(renderer, 0x1E, 0x1E, 0x23, 0xFF);
    SDL_RenderClear(renderer);

    renderUI();
  } else if (isGenerating) {
    showLoadingScreen();
  } else {
    highlightGrid();
    showGridLines();
    showNumbers();
    showNumberStats();
    showHud();
    if (isPaused) showPauseScreen();
    if (isPuzzleSolved) showWinScreen();
  }
  SDL_RenderPresent(renderer);
}

void Sudoku::renderUI() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xB4);
  SDL_Rect shade = { 0, 0, windowWidth, windowHeight };
  SDL_RenderFillRect(renderer, &shade);

  ImGui::NewFrame();

  ImVec2 menuSize = ImVec2(windowWidth * 0.6f, windowHeight * 0.5f);
  ImGui::SetNextWindowPos(ImVec2(windowWidth * 0.5f, windowHeight * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(menuSize, ImGuiCond_Always);
  ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

  float windowWidthIm = ImGui::GetWindowSize().x;
  float buttonWidth = 250.f;

  ImGui::SetWindowFontScale(2.5f);
  float titleWidth = ImGui::CalcTextSize("SUDOKU PRO").x;
  ImGui::SetCursorPosX((windowWidthIm - titleWidth) * 0.5f);
  ImGui::Text("SUDOKU PRO");
  ImGui::SetWindowFontScale(1.f);

  ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();


  ImGui::SetWindowFontScale(2.f);
  float difficultyWidth = ImGui::CalcTextSize("Select Difficulty").x;
  ImGui::SetCursorPosX((windowWidthIm - difficultyWidth) * 0.5f);
  ImGui::Text("Select Difficulty"); 
  ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
  ImGui::SetWindowFontScale(1.5f);

  ImGui::SetCursorPosX((windowWidthIm - ImGui::CalcTextSize("Easy").x - 10.f) * 0.5f);
  if (ImGui::RadioButton("Easy", targetClues == 45)) targetClues = 45;

  ImGui::SetCursorPosX((windowWidthIm - ImGui::CalcTextSize("Medium").x - 10.f) * 0.5f);
  if (ImGui::RadioButton("Medium", targetClues == 35)) targetClues = 35;

  ImGui::SetCursorPosX((windowWidthIm - ImGui::CalcTextSize("Hard").x - 10.f) * 0.5f);
  if (ImGui::RadioButton("Hard", targetClues == 25)) targetClues = 25;

  ImGui::Dummy(ImVec2(0.f, 50.f));

  ImGui::Spacing();

  ImGui::SetCursorPosX((windowWidthIm - buttonWidth) * 0.5f);
  if (ImGui::Button("START NEW GAME", ImVec2(buttonWidth, 0))) {
    isShowingMenu = false;
    initializeNewGame(true);
  }

  ImGui::SetCursorPosX((windowWidthIm - buttonWidth) * 0.5f);
  if (ImGui::Button("QUIT", ImVec2(buttonWidth, 0))) {
    isRunning = false;
  }

  ImGui::SetWindowFontScale(1.f);
  ImGui::End();

  ImGui::Render();
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void Sudoku::update() {
  if (isGenerating) {
    SDL_Delay(16);
    return;
  }

  if (!isPuzzleSolved && !isPaused) {
    auto now = std::chrono::steady_clock::now();
    auto totalSinceStart = now - startTime;
    finalElapsedDuration = std::chrono::duration_cast<std::chrono::seconds>(totalSinceStart - totalPausedDuration);
  }
}

int Sudoku::run() {
  if (!init()) {
    std::cerr << "Failed to initialize\n";
    return 1;
  }

  initializeNewGame(true);
  while (isRunning) {
    update();
    handleEvents();
    render();
  }
  return 0;
}

void Sudoku::initializeNewGame(bool generateNew) {
  board = fixedBoard = solvedBoard = {};
  isPaused = isPuzzleSolved = false;
  selectedRow = selectedCol = -1;
  mistakeCount = 0;

  startTime = std::chrono::steady_clock::now();
  finalElapsedDuration = std::chrono::seconds(0);

  if (generateNew) {
    isGenerating = true; 
    if (generationThread.joinable()) generationThread.join();
    generationThread = std::thread(&Sudoku::generatePuzzleTask, this);
  }
}

void Sudoku::generatePuzzle() {
  static std::random_device rd;
  static std::default_random_engine dre(rd());

  selectedRow = dre() % 9;
  selectedCol = dre() % 9;
  
  std::vector<int> nums = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  std::shuffle(nums.begin(), nums.end(), dre);

  int index = 0;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      board[r][c] = nums[index++];
    }
  }
  
  NumberGrid tempBoard = {};
  solveSudoku(tempBoard);

  solvedBoard = tempBoard;
  board = tempBoard;
  fixedBoard = tempBoard;

  std::vector<std::pair<int, int>> cells;
  for (int r = 0; r < BOARD_DIM; ++r) {
    for (int c = 0; c < BOARD_DIM; ++c) {
      cells.emplace_back(r, c);
    }
  }
  std::shuffle(cells.begin(), cells.end(), dre);
  
  int removedCount = 0;
  int maxRemovals = BOARD_DIM * BOARD_DIM - targetClues;

  for (const auto &cell : cells) {
    if (removedCount >= maxRemovals) break;
    int r = cell.first;
    int c = cell.second;

    if (board[r][c] == 0) continue;

    int backupVal = board[r][c];
    board[r][c] = 0;

    if (countSolutions(board) == 1) {
      ++removedCount;
    } else {
      board[r][c] = backupVal;
    }
  }

  fixedBoard = board;
}

void Sudoku::calculateLayout() {
  SDL_GetWindowSize(window, &windowWidth, &windowHeight);

  int minDim = std::min(windowWidth, windowHeight);
  boardSize = static_cast<int>(minDim * 0.8f);
  cellSize = boardSize / 9;

  boardSize = cellSize * 9;

  offsetX = (windowWidth - boardSize) / 2;
  offsetY = (windowHeight - boardSize) / 2;
}

void Sudoku::handleMouseEvents() {
  int oldRow = selectedRow;
  int oldCol = selectedCol;

  int mouseX, mouseY;
  SDL_GetMouseState(&mouseX, &mouseY);
  if (mouseX >= offsetX && mouseX < offsetX + boardSize &&
      mouseY >= offsetY && mouseY < offsetY + boardSize) {
    selectedCol = (mouseX - offsetX) / cellSize;
    selectedRow = (mouseY - offsetY) / cellSize;
  } else {
    selectedRow = -1;
    selectedCol = -1;
  }

  if (selectedRow != oldRow || selectedRow != oldCol) {
    Mix_PlayChannel(-1, moveSound, 0);
  }
}

void Sudoku::handleKeyboardEvents(int key) {
  if (key == SDLK_ESCAPE) {
    isRunning = false;
    return;
  }

  if (isShowingMenu) {
    handleDifficultyKeys(key);

    if ((key == SDLK_n || key == SDLK_RETURN) && !isGenerating) {
      isShowingMenu = false;
      initializeNewGame(true);

      startTime = std::chrono::steady_clock::now();
      totalPausedDuration = std::chrono::seconds(0);
      finalElapsedDuration = std::chrono::seconds(0);
    }
    return;
  }

  if (key == SDLK_n) {
    isShowingMenu = true;
    isPaused = false;
    isPuzzleSolved = false;

    selectedRow = -1;
    selectedCol = -1;
    return;
  }

  if (key == SDLK_p || key == SDLK_SPACE) {
    if (!isPuzzleSolved) {
      isPaused = !isPaused;

      if (isPaused) {
        pauseStartTime = std::chrono::steady_clock::now();
        selectedRow = -1;
        selectedCol = -1;
      } else {
        auto now = std::chrono::steady_clock::now();
        auto pauseSegment = std::chrono::duration_cast<std::chrono::seconds>(now - pauseStartTime);
        totalPausedDuration += pauseSegment;
      }
    }
    return;
  }

  if (!isPaused && !isPuzzleSolved) {
    if (key == SDLK_s) {
      if (solveSudoku(board)) {
        isPuzzleSolved = true;
        std::cout << "Puzzle solved successfully.\n";
      } else {
        std::cout << "Puzzle is unsolvable.\n";
      }
    }

    handleNumericKeys(key);
    handleArrowKeys(key);
  }
}

void Sudoku::handleArrowKeys(int key) {
  int oldRow = selectedRow;
  int oldCol = selectedCol;

  if (key == SDLK_LEFT) {
    if (selectedCol - 1 > -1) --selectedCol;
  } else if (key == SDLK_RIGHT) {
    if (selectedCol + 1 < 9) ++selectedCol;
  } else if (key == SDLK_UP) {
    if (selectedRow - 1 > -1) --selectedRow;
  } else if (key == SDLK_DOWN) {
    if (selectedRow + 1 < 9) ++selectedRow;
  }

  if (selectedRow != oldRow || selectedCol != oldCol) {
    Mix_PlayChannel(-1, moveSound, 0);
  }
}

void Sudoku::handleNumericKeys(int key) {
  int enteredValue = -1;
  if (selectedRow != -1 && selectedCol != -1) {
    if (key >= SDLK_1 && key <= SDLK_9) {
      enteredValue = key - SDLK_0;
    } else if (key == SDLK_DELETE || key == SDLK_BACKSPACE) {
      enteredValue = 0;
    }

    if (enteredValue >= 0 && enteredValue <= 9) {
      if (fixedBoard[selectedRow][selectedCol] == 0) {
        board[selectedRow][selectedCol] = enteredValue;
        
        if (enteredValue != solvedBoard[selectedRow][selectedCol] && enteredValue != 0) {
          ++mistakeCount;
          Mix_PlayChannel(-1, mistakeSound, 0);
        } else {
          Mix_PlayChannel(-1, inputSound, 0);
        }

        if (isSolved(board)) {
          isPuzzleSolved = true;
          Mix_PlayChannel(-1, winSound, 0);
        }
      }
    }
  }
}

void Sudoku::handleDifficultyKeys(int key) {
  const int oldClues = targetClues;
  
  if      (key == SDLK_1) targetClues = 40;
  else if (key == SDLK_2) targetClues = 30;
  else if (key == SDLK_3) targetClues = 22;
  else if (key == SDLK_4) targetClues = 17;
  
  if (targetClues != oldClues) {
    initializeNewGame(true);
  }
}

void Sudoku::highlightGrid() {
  if (selectedRow == -1 || selectedCol == -1) return;

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  SDL_SetRenderDrawColor(renderer, 0xE0, 0xE0, 0xFF, 0x60);
  SDL_Rect rowRect = { offsetX, offsetY + selectedRow * cellSize, boardSize, cellSize };
  SDL_Rect colRect = { offsetX + selectedCol * cellSize, offsetY, cellSize, boardSize };
  SDL_RenderFillRect(renderer, &rowRect);
  SDL_RenderFillRect(renderer, &colRect);

  int boxStartRow = (selectedRow / 3) * 3;
  int boxStartCol = (selectedCol / 3) * 3;
  SDL_Rect boxRect = {
    offsetX + boxStartCol * cellSize,
    offsetY + boxStartRow * cellSize,
    cellSize * 3, cellSize * 3
  };

  SDL_RenderFillRect(renderer, &boxRect);

  int selectedValue = board[selectedRow][selectedCol];
  if (selectedValue != 0) {
    SDL_SetRenderDrawColor(renderer, 0xBB, 0xDE, 0xFB, 0xA0);
    for (int r = 0; r < BOARD_SIZE; ++r) {
      for (int c = 0; c < BOARD_SIZE; ++c) {
        if (board[r][c] == selectedValue) {
          SDL_Rect matchRect = { offsetX + c * cellSize, offsetY + r * cellSize, cellSize, cellSize };
          SDL_RenderFillRect(renderer, &matchRect);
        }
      }
    }
  }

  SDL_SetRenderDrawColor(renderer, 0x90, 0xEE, 0x90, 0xFF);
  SDL_Rect focusRect = { 
    offsetX + selectedCol * cellSize,
    offsetY + selectedRow * cellSize,
    cellSize, cellSize
  };
  SDL_RenderFillRect(renderer, &focusRect);

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Sudoku::showGridLines() {
  if (selectedRow != -1 && selectedCol != -1) {
    SDL_SetRenderDrawColor(renderer, 0x90, 0xEE, 0x90, 0xFF);
    SDL_Rect highlightRect = {
      offsetX + selectedCol * cellSize,
      offsetY + selectedRow * cellSize,
      cellSize, cellSize
    };
    SDL_RenderFillRect(renderer, &highlightRect);
  }
  
  SDL_SetRenderDrawColor(renderer, 0xAA, 0xAA, 0xAA, 0xFF);
  for (int i = 0; i <= BOARD_SIZE; ++i) {
    int xPos = offsetX + i * cellSize;
    int yPos = offsetY + i * cellSize;

    SDL_RenderDrawLine(renderer, xPos, offsetY, xPos, offsetY + boardSize);
    SDL_RenderDrawLine(renderer, offsetX, yPos, offsetX + boardSize, yPos);
  }

  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  for (int i = 0; i <= BOARD_SIZE; i += SUBGRID_SIZE) {
    int xPos = offsetX + i * cellSize;
    int yPos = offsetY + i * cellSize;

    for (int t = -1; t <= 1; ++t) {
      SDL_RenderDrawLine(renderer, xPos + t, offsetY, xPos + t, offsetY + boardSize);
      SDL_RenderDrawLine(renderer, offsetX, yPos + t, offsetX + boardSize, yPos + t);
    }
  }
}

Color Sudoku::getCellColor(int row, int col) {
  int currentValue = board[row][col];

  if (fixedBoard[row][col] != 0) {
    return { 0x00, 0x00, 0x00, 0xFF };
  }

  if (currentValue != 0) {
    if (currentValue == solvedBoard[row][col]) {
      return { 0x00, 0x00, 0xFF, 0x00 };
    } else {
      return { 0xFF, 0x00, 0x00, 0xFF };
    }
  }

  return { 0x00, 0x00, 0x00, 0xFF };
}

void Sudoku::showNumbers() {
  //int responsiveFontSize = static_cast<int>(cellSize * 0.75f);
  TTF_SetFontSize(font, FONT_SIZE);

  for (int row = 0; row < BOARD_DIM; ++row) {
    for (int col = 0; col < BOARD_DIM; ++col) {
      int value = board[row][col];
      if (value == 0) continue;

      const Color textColor = getCellColor(row, col);

      char numText[2] = {0};
      numText[0] = (char)(value + (int)'0');

      SDL_Texture *textTexture = textureManager->getTextTexture(numText, font, textColor);

      int w, h;
      SDL_QueryTexture(textTexture, nullptr, nullptr, &w, &h);

      SDL_Rect renderQuad = {
        offsetX + col * cellSize + (cellSize - w) / 2,
        offsetY + row * cellSize + (cellSize - h) / 2,
        w, h
      };

      SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    }
  }
}

void Sudoku::showHud() {
  auto totalSeconds = finalElapsedDuration.count();
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  std::string diffText;
  
  if      (targetClues >= 40) diffText += "Easy";
  else if (targetClues >= 30) diffText += "Medium";
  else if (targetClues >= 20) diffText += "Hard";
  else                        diffText += "Expert";

  std::string hudString = std::format("Mistakes: {} | Time: {:02d}:{:02d} | Level: {}",
                                      mistakeCount, minutes, seconds, diffText);
  
  SDL_Color hudColor = { 0x8F, 0x00, 0xFF, 0xFF };

  TTF_SetFontSize(font, 20);
  SDL_Surface *surface = TTF_RenderText_Blended(font, hudString.c_str(), hudColor);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect rect = {
    (windowWidth - surface->w) / 2,
    (offsetY - surface->h) / 2,
    surface->w, surface->h
  };

  SDL_RenderCopy(renderer, texture, nullptr, &rect);
  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
  TTF_SetFontSize(font, FONT_SIZE);
}

void Sudoku::showPauseScreen() {
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xCC);
  SDL_Rect screenRect = { 0, 0, windowWidth, windowHeight };
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(renderer, &screenRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  SDL_Color pauseColor = { 0xFF, 0xFF, 0xFF, 0xFF };
  constexpr char pauseText[] = "PAUSED!";
  constexpr char subText[] = "Press 'P' or Space to Continue.";

  SDL_Surface *pauseSurface = TTF_RenderText_Blended(font, pauseText, pauseColor);
  SDL_Texture *pauseTexture = SDL_CreateTextureFromSurface(renderer, pauseSurface);
  SDL_Rect pauseQuad = {
    (windowWidth - pauseSurface->w) / 2,
    (windowHeight / 2) - 50,
    pauseSurface->w, pauseSurface->h
  };

  SDL_RenderCopy(renderer, pauseTexture, nullptr, &pauseQuad);
  SDL_FreeSurface(pauseSurface);
  SDL_DestroyTexture(pauseTexture);

  TTF_SetFontSize(font, 20);
  SDL_Surface *subSurface = TTF_RenderText_Blended(font, subText, pauseColor);
  SDL_Texture *subTexture = SDL_CreateTextureFromSurface(renderer, subSurface);
  SDL_Rect subQuad = {
    (windowWidth - subSurface->w) / 2,
    (windowHeight / 2) + 20,
    subSurface->w, subSurface->h
  };

  SDL_RenderCopy(renderer, subTexture, nullptr, &subQuad);
  SDL_FreeSurface(subSurface);
  SDL_DestroyTexture(subTexture);
  TTF_SetFontSize(font, FONT_SIZE);
}

void Sudoku::showWinScreen() {
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xCC);
  SDL_Rect screenRect = { 0, 0, windowWidth, windowHeight };

  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(renderer, &screenRect);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

  SDL_Color winColor = { 0x00, 0x80, 0xFF, 0xFF };
  constexpr char winText[] = "YOU WIN!";
  constexpr char subText[] = "Press 'N' for New Game, Esc to Quit.";

  SDL_Surface *winSurface = TTF_RenderText_Blended(font, winText, winColor);
  SDL_Texture *winTexture = SDL_CreateTextureFromSurface(renderer, winSurface);
  SDL_Rect winQuad = {
    (windowWidth - winSurface->w) / 2,
    (windowHeight / 2) - 50,
    winSurface->w, winSurface->h
  };

  SDL_RenderCopy(renderer, winTexture, nullptr, &winQuad);
  SDL_FreeSurface(winSurface);
  SDL_DestroyTexture(winTexture);

  SDL_Surface *subSurface = TTF_RenderText_Blended(font, subText, winColor);
  SDL_Texture *subTexture = SDL_CreateTextureFromSurface(renderer, subSurface);
  SDL_Rect subQuad = {
    (windowWidth - subSurface->w) / 2,
    (windowHeight / 2) + 20,
    subSurface->w, subSurface->h
  };

  SDL_RenderCopy(renderer, subTexture, nullptr, &subQuad);
  SDL_FreeSurface(subSurface);
  SDL_DestroyTexture(subTexture);
}

void Sudoku::showLoadingScreen() {
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x8C);
  SDL_Rect fullScreen = { 0, 0, windowWidth, windowHeight };
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(renderer, &fullScreen);

  auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  float pulse = (std::sin(now / 100'000'000.f) + 1.f) / 2.f;
  Uint8 alpha = static_cast<Uint8>(150 + (105 * pulse));

  SDL_Color textColor = { 0xFF, 0xFF, 0xFF, alpha };
  TTF_SetFontSize(font, 28);

  const char message[] = "Generating Puzzle...";
  SDL_Surface *surface = TTF_RenderText_Blended(font, message, textColor);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_Rect textQuad = {
    (windowWidth - surface->w) / 2,
    (windowHeight - surface->h) / 2,
    surface->w, surface->h
  };

  SDL_RenderCopy(renderer, texture, nullptr, &textQuad);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
  TTF_SetFontSize(font, FONT_SIZE);
}

void Sudoku::showNumberStats() {
  int counts[10] = {0};
  for (int r = 0; r < 9; ++r) {
    for (int c = 0; c < 9; ++c) {
      int val = board[r][c];
      if (val == solvedBoard[r][c] && val != 0) {
        ++counts[val];
      }
    }
  }

  const int statsY = offsetY + boardSize + 20;
  const int spacing = windowWidth / 10;
  TTF_SetFontSize(font, 24);

  for (int i = 1; i <= 9; ++i) {
    int remaining = 9 - counts[i];

    SDL_Color color;
    if (remaining == 0) color = { 0x00, 0xAA, 0x00, 0xFF };
    else color = { 0x66, 0x66, 0x66, 0xFF };
    
    char numStr[2] = {};
    numStr[0] = (char)('0' + i);
    SDL_Surface *surfaceNum = TTF_RenderText_Blended(font, numStr, color);
    SDL_Texture *textureNum = SDL_CreateTextureFromSurface(renderer, surfaceNum);

    TTF_SetFontSize(font, 14);
    std::string countStr = (remaining == 0) ? "Done" : "x" + std::to_string(remaining);
    SDL_Surface *surfaceCount = TTF_RenderText_Blended(font, countStr.c_str(), color);
    SDL_Texture *textureCount = SDL_CreateTextureFromSurface(renderer, surfaceCount);

    int xPos = (i * spacing) - (surfaceNum->w / 2);
    SDL_Rect rectNum = { xPos, statsY, surfaceNum->w, surfaceNum->h };
    SDL_Rect rectCount = { xPos - 5, statsY + 30, surfaceCount->w, surfaceCount->h };

    SDL_RenderCopy(renderer, textureNum, nullptr, &rectNum);
    SDL_RenderCopy(renderer, textureCount, nullptr, &rectCount);

    SDL_FreeSurface(surfaceNum);
    SDL_DestroyTexture(textureNum);
    SDL_FreeSurface(surfaceCount);
    SDL_DestroyTexture(textureCount);

    TTF_SetFontSize(font, 24);
  }
  TTF_SetFontSize(font, FONT_SIZE);
}

void Sudoku::generatePuzzleTask() {
  auto taskStart = std::chrono::steady_clock::now();

  generatePuzzle();
  fixedBoard = board;

  auto taskEnd = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(taskEnd - taskStart);

  if (elapsed < std::chrono::milliseconds(350)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(350) - elapsed);
  }
  
  isGenerating = false;
}
