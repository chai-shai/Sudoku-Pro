# Sudoku-Pro 🧩

A modern, high-performance Sudoku game engine built with **C++20**, **SDL2**, and **Dear ImGui**. This project features a custom-built backtracking generation engine and a responsive UI designed for a smooth puzzle-solving experience.

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform: Windows | Linux | macOS](https://img.shields.io/badge/Platform-Cross--Platform-blue)

---

## 🚀 Key Features

* **Dynamic Generation:** Puzzles are generated on-the-fly with a backtracking algorithm, ensuring every board has exactly one unique solution.
* **Modern UI Stack:** Powered by **Dear ImGui** for a professional, responsive menu system and game overlays.
* **Intelligent Sizing:** All UI elements and grid components are content-aware and scale dynamically to window resizing.
* **Audio Feedback:** Immersive sound effects for cell movement, number input, mistakes, and victory via **SDL_mixer**.
* **Asynchronous Generation:** Puzzle logic runs on a dedicated background thread to prevent UI "freezing" during complex board calculations.
* **Visual Assistance:** Highlighting system for selected rows, columns, sub-grids, and matching numbers to aid gameplay.

---

## 🛠️ Tech Stack

| Component | Technology |
| :--- | :--- |
| **Language** | C++20 |
| **Graphics** | SDL2 (Hardware Accelerated) |
| **User Interface** | Dear ImGui |
| **Text/Fonts** | SDL_ttf |
| **Audio** | SDL_mixer (OGG Support) |

---

## 🎮 How to Play

### Controls
* **Mouse:** Click cells to select. Use the ImGui menu to toggle difficulty and start games.
* **Numbers (1-9):** Input values into the selected cell.
* **Backspace/Delete:** Clear the value of a non-fixed cell.
* **Arrow Keys:** Navigate the grid with haptic sound feedback.
* **Space / P:** Pause the game and hide the grid (Cheating prevention).
* **N:** Return to the Main Menu.
* **S:** Solve the current puzzle (Solver demonstration).

### Difficulty Levels
* **Easy:** 45 clues
* **Medium:** 35 clues
* **Hard:** 25 clues

---

## 📥 Installation & Building

### Prerequisites
You will need the following development libraries:
* `SDL2`
* `SDL2_ttf`
* `SDL2_mixer`

### Compiling Sudoku-Pro:

```bash
mkdir build
cd build
cmake ..
