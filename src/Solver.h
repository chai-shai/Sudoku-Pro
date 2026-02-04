#pragma once
#ifndef SOLVER_H
#define SOLVER_H

#include "Common.h"

bool isConflicting(const NumberGrid &board, int row, int col);
bool isSolved(const NumberGrid &board);
bool findEmptyCell(const NumberGrid &board, int &row, int &col);
bool solveSudoku(NumberGrid &board);
int countSolutions(NumberGrid &board);
void solveAndCount(NumberGrid &board, int &count);

#endif //!SOVER_H
