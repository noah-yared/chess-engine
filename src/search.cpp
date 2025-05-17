#include <algorithm>
#include <limits>
#include <iostream>
#include <memory>
 
#include "movegen.h"
#include "search.h"
#include "evaluate.h"
#include "board.hpp"
#include "Move.h"
#include "sides.h"


Evaluation::Score alphaBeta(Board* node, int alpha, int beta, int depth, bool isMaximizingPlayer, Side side) {

  if (!depth) {
    return Evaluation::Evaluator::evaluate(*node);
  }

  auto possibleMoves = generateMoves(node, side);

  if (possibleMoves.empty()) {
    std::cout << "no possible moves -- game over!" << std::endl;
    return Evaluation::Evaluator::evaluate(*node);
  }

  if (isMaximizingPlayer) { // white
    for (const auto& move : possibleMoves) {
      auto newNode = Board(*node);
      newNode.makeMove(move.get());
      alpha = std::max(alpha, alphaBeta(&newNode, alpha, beta, depth-1, false, side == WHITE ? BLACK : WHITE));
      if (beta <= alpha) break;
    }
    return alpha;
  } else {
    for (const auto& move : possibleMoves) {
      auto newNode = Board(*node);
      newNode.makeMove(move.get());
      beta = std::min(beta, alphaBeta(&newNode, alpha, beta, depth-1, true, side == WHITE ? BLACK : WHITE));
      if (beta <= alpha) break;
    }
    return beta;
  }
}