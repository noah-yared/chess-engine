#include "board.hpp"
#include "sides.h"
#include "pieces.h"
#include "movegen.h"
#include "evaluate.h"
#include "search_utils.h"
#include "search.h"
#include "utils.h"
#include "main.h"

#include <cassert>
#include <iostream>

// Uncomment for debugging mode
// #define DEBUG

// Uncomment for testing
// #define TEST

static const int SEARCH_DEPTH = 3;

// static const auto nullMove = Move(0, 0, static_cast<Pieces::piece>(0));
static const auto nullMove = Move();

static bool isMoveNull(Move* move) {
  return move->i() == nullMove.i() && move->f() == nullMove.f() && move->p() == nullMove.p();
}

static SearchPath stateHistory;

Move pickMove(Board* node, Side side) {
  auto possibleMoves = generateMoves(node, side);
  int bestScore = side == WHITE ? NEGINF : POSINF;
  Move bestMove = nullMove;
  for (const auto& move : possibleMoves) {
    assert(move);
    stateHistory.push(node->pullState());
    node->makeMove(move.get());
    int score = alphaBeta(node, NEGINF, POSINF, SEARCH_DEPTH, false, side == WHITE ? BLACK : WHITE);
    node->undoMove(move.get(), stateHistory.pop());
    if ((side == WHITE && score > bestScore) || (side == BLACK && score < bestScore)) {
      bestScore = score;
      bestMove = *move;
    }
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
    for (auto& move : generateMoves(&board, BLACK)) {
      printMove(*move);
    }
  #else
    std::cout << "Debug mode disabled" << std::endl;

    Board testBoard = Board();
    Side currSide = WHITE;

    int simulatedMovesLimit = 10;
    int movesSimulated = 0;

    auto bestMove = pickMove(&testBoard, currSide);
    while (!isMoveNull(&bestMove) && movesSimulated < simulatedMovesLimit) {
      testBoard.makeMove(&bestMove);
      std::cout << "Move " << std::dec << (++movesSimulated) << ": "; printMove(bestMove);
      std::cout << "State: 0x" << std::hex << testBoard.pullState() << std::endl;
      testBoard.printBoard();
      currSide = (currSide == WHITE) ? BLACK : WHITE;
      bestMove = pickMove(&testBoard, currSide);
    }

    std::cout << "Finished simulating " << std::dec << movesSimulated << " move" << (movesSimulated == 1 ? "" : "s") << "!" << std::endl;
    std::cout << "Explored " << std::dec << NODES_EXPLORED << " nodes!" << std::endl;
  #endif
  return 0;
}
#endif