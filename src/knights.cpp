#include "knights.h"

#include <memory>
#include <vector>

#include "board.hpp"
#include "Move.h"
#include "kingSafety.h"
#include "pieces.h"
#include "sides.h"

std::vector<std::unique_ptr<Move>> Knight::generateMoves(Board *board,
                                                         Side side) {
  std::vector<std::unique_ptr<Move>> knightMoves;

  ull knightsBB = board->readBB(side == WHITE ? Pieces::N : Pieces::n);
  ull allyBB = side == WHITE ? board->readWhiteBB() : board->readBlackBB();

  while (knightsBB) {
    int knightIndex = __builtin_ctzll(knightsBB);
    ull knightAttacks = board->knightAttacks()[knightIndex] & ~allyBB;
    while (knightAttacks) {
      int attackIndex = __builtin_ctzll(knightAttacks);
      Move move(knightIndex, attackIndex, side == WHITE ? Pieces::N : Pieces::n);
      if (!Attack::doesMoveExposeAllyKingToCheck(board, &move, side)) {
        if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
          move.setFlag(Flags::CHECK);
        }
        knightMoves.emplace_back(std::make_unique<Move>(move));
      }
      knightAttacks &= (knightAttacks - 1);
    }
    knightsBB &= (knightsBB - 1);
  }

  return knightMoves;
}