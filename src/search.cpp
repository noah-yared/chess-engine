#include <algorithm>
#include <limits>
#include <iostream>
#include <memory>
 
#include "movegen.h"
#include "search.h"
#include "search_utils.h"
#include "evaluate.h"
#include "board.hpp"
#include "Move.h"
#include "sides.h"


SearchPath stateHistory;

Evaluation::Score alphaBeta(Board* node, int alpha, int beta, int depth, bool isMaximizingPlayer, Side side) {

  if (!depth) {
    return Evaluation::Evaluator::evaluate(*node);
  }

  auto possibleMoves = generateMoves(node, side);
  NODES_EXPLORED += possibleMoves.size();

  if (possibleMoves.empty()) {
    return Evaluation::Evaluator::evaluate(*node);
  }

  if (isMaximizingPlayer) { // white
    for (const auto& move : possibleMoves) {
      // save state to history
      stateHistory.push(node->pullState());
      node->makeMove(move.get());
      alpha = std::max(alpha, alphaBeta(node, alpha, beta, depth-1, false, side == WHITE ? BLACK : WHITE));
      node->undoMove(move.get(), stateHistory.pop());
      if (beta <= alpha) break;
    }
    return alpha;
  } else {
    for (const auto& move : possibleMoves) {
      // save state to history
      stateHistory.push(node->pullState());
      node->makeMove(move.get());
      beta = std::min(beta, alphaBeta(node, alpha, beta, depth-1, true, side == WHITE ? BLACK : WHITE));
      node->undoMove(move.get(), stateHistory.pop());
      if (beta <= alpha) break;
    }
    return beta;
  }
}