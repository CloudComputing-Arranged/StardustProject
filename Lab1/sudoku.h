#ifndef SUDOKU_H
#define SUDOKU_H

const bool DEBUG_MODE = false;
enum { ROW=9, COL=9, N = 81, NEIGHBOR = 20 };
const int NUM = 9;

extern int neighbors[10][N][NEIGHBOR];
extern int board[10][N];
extern int spaces[10][N];
extern int nspaces[10];
extern int (*chess)[10][COL];

void init_neighbors(int num);
void input(const char in[N], int num);
void init_cache();

bool available(int guess, int cell);

bool solve_sudoku_basic(int which_space, int num);
bool solve_sudoku_min_arity(int which_space);
bool solve_sudoku_min_arity_cache(int which_space);
bool solve_sudoku_dancing_links(int unused);
bool solved(int num);
#endif
