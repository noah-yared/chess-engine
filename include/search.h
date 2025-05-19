#pragma once

#include "board.hpp"
#include "sides.h"
#include "evaluate.h"

#define NEGINF std::numeric_limits<int>::min()
#define POSINF std::numeric_limits<int>::max()

inline ull NODES_EXPLORED = 0;

Evaluation::Score alphaBeta(Board* node, int alpha, int beta, int depth, bool isMaximizingPlayer, Side side);