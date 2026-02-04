#include "Solver.h"

bool isConflicting(const NumberGrid &board, int row, int col) {
  const int value = board[row][col];
  if (value == 0) {
    return false;
  }

  for (int c = 0; c < BOARD_DIM; ++c) {
    if (c != col && board[row][c] == value) {
      return true;
    }
  }

  for (int r = 0; r < BOARD_DIM; ++r) {
    if (r != row && board[r][col] == value) {
      return true;
    }
  }

  int startRow = row - row % SUBGRID_SIZE;
  int startCol = col - col % SUBGRID_SIZE;

  for (int r = 0; r < SUBGRID_SIZE; ++r) {
    for (int c = 0; c < SUBGRID_SIZE; ++c) {
      int currRow = startRow + r;
      int currCol = startCol + c;

      if (currRow != row || currCol != col) {
        if (board[currRow][currCol] == value) {
          return true;
        }
      }
    }
  }
  return false;
}

bool isSolved(const NumberGrid &board) {
  bool full = true;
  for (int r = 0; r < BOARD_DIM; ++r) {
    for (int c = 0; c < BOARD_DIM; ++c) {
      if (board[r][c] == 0) {
        full = false;
      }

      if (isConflicting(board, r, c)) {
        return false;
      }
    }
  }
  return full;
}

bool findEmptyCell(const NumberGrid &board, int &row, int &col) {
  for (row = 0; row < BOARD_DIM; ++row) {
    for (col = 0; col < BOARD_DIM; ++col) {
      if (board[row][col] == 0) {
        return true;
      }
    }
  }
  return false;
}

bool solveSudoku(NumberGrid &board) {
  int row, col;
  if (!findEmptyCell(board, row, col)) {
    return true;
  }

  for (int num = 1; num <= 9; ++num) {
    board[row][col] = num;
    if (!isConflicting(board, row, col)) {
      if (solveSudoku(board)) {
        return true;
      }
    }
  }

  board[row][col] = 0;
  return false;
}

int countSolutions(NumberGrid &board) {
  int count = 0;
  solveAndCount(board, count);
  return count;
}

void solveAndCount(NumberGrid &board, int &count) {
  if (count >= 2) {
    return;
  }

  int row, col;
  if (!findEmptyCell(board, row, col)) {
    ++count;
    return;
  }

  for (int num = 1; num <= 9; ++num) {
    board[row][col] = num;
    if (!isConflicting(board, row, col)) {
      solveAndCount(board, count);
    }
    board[row][col] = 0;
  }
}
