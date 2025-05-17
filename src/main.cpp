#include "board.hpp"
#include "sides.h"
#include "pieces.h"
#include "movegen.h"
#include "evaluate.h"
#include "utils.h"
#include "main.h"

#include <iostream>

// Uncomment for debugging mode
// #define DEBUG

// Uncomment for testing
// #define TEST


Move pickMove(Board* node, Side side) {
  auto possibleMoves = generateMoves(node, side);
  int bestScore = side == WHITE ? NEGINF : POSINF;
  Move bestMove = Move(0, 0, static_cast<Pieces::piece>(0));
  for (const auto& move : possibleMoves) {
    node->makeMove(move.get());
    int score = alphaBeta(node, NEGINF, POSINF, 3, false, side == WHITE ? BLACK : WHITE);
    if (side == WHITE && score > bestScore || side == BLACK && score < bestScore) {
      bestScore = score;
      if (!move) {std::cerr << "Move pointer is null!" << std::endl; exit(1);}
      bestMove = *move;
    }
    node->undoMove(move.get());
  }
  return bestMove;
}

#ifdef TEST
#else
int main() {
  #ifdef DEBUG
    std::cout << "Debug mode enabled" << std::endl;
    // put this into the board
    // R.BQKB.R
    // PPPPPPPP
    // ..N..N..
    // ........
    // ....p...
    // ........
    // pppp.ppp
    // rnbqkbnr
    std::array<std::array<char, 8>, 8> sboard = {{
      {{'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'}},
      {{'p', 'p', 'p', 'p', '.', 'p', 'p', 'p'}},
      {{'.', '.', '.', '.', '.', '.', '.', '.'}},
      {{'.', '.', '.', '.', 'p', '.', '.', '.'}},
      {{'.', '.', '.', '.', '.', '.', '.', '.'}},
      {{'.', '.', 'N', '.', '.', 'N', '.', '.'}},
      {{'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'}},
      {{'R', '.', 'B', 'Q', 'K', 'B', '.', 'R'}}}};
    ull bbs[12] = {0ULL};
    toBitboard(sboard, bbs);
    auto board = Board(bbs, {'k', 'q'}, std::nullopt, 59, 3, false, false, 0, 0);
    // std::cout << (board.getAttackers(BLACK) & board.readBlackBB()) << std::endl;
    for (auto& move : generateMoves(&board, BLACK)) {
      printMove(*move);
    }
  #else
    std::cout << "Debug mode disabled" << std::endl;

    Board testBoard = Board();
    Side currSide = WHITE;
    auto bestMove = pickMove(&testBoard, currSide);
    int move_ct = 1;
    while (bestMove.i() != bestMove.f()) {
      testBoard.makeMove(&bestMove);
      std::cout << "Move " << move_ct << ": "; printMove(bestMove);
      testBoard.printBoard();
      move_ct++;
      if (move_ct > 10) {std::cout << "simulated 10 move(s)!" << std::endl; break;}
      currSide = (currSide == WHITE) ? BLACK : WHITE;
      bestMove = pickMove(&testBoard, currSide);
    }
  #endif
  
}
#endif