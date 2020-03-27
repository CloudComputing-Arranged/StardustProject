#include <assert.h>
#include <stdio.h>

#include <algorithm>

#include "sudoku.h"

int board[10][N];   //所有全局变量都增设为10组
int spaces[10][N];
int nspaces[10];
int (*chess)[10][COL] = (int (*)[10][COL])board;

static void find_spaces(int num)
{
  nspaces[num] = 0;
  for (int cell = 0; cell < N; ++cell) {
    if (board[num][cell] == 0)
      spaces[num][nspaces[num]++] = cell;
  }
}

void input(const char in[N], int num)
{
  for (int cell = 0; cell < N; ++cell) {
    board[num][cell] = in[cell] - '0';
    assert(0 <= board[num][cell] && board[num][cell] <= NUM);
  }
  find_spaces(num);
}

bool available(int guess, int cell, int num)
{
  for (int i = 0; i < NEIGHBOR; ++i) {
    int neighbor = neighbors[num][cell][i];
    if (board[num][neighbor] == guess) {
      return false;
    }
  }
  return true;
}

bool solve_sudoku_basic(int which_space, int num)
{
  if (which_space >= nspaces[num]) {
    return true;
  }

  // find_min_arity(which_space);
  int cell = spaces[num][which_space];

  for (int guess = 1; guess <= NUM; ++guess) {
    if (available(guess, cell, num)) {
      // hold
      assert(board[num][cell] == 0);
      board[num][cell] = guess;

      // try
      if (solve_sudoku_basic(which_space+1, num)) {
        return true;
      }

      // unhold
      assert(board[num][cell] == guess);
      board[num][cell] = 0;
    }
  }
  return false;
}
