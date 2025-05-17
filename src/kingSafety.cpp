#include "kingSafety.h"
#include "directions.h"
#include "Move.h"
#include "utils.h"

#include <cassert>

namespace {
  constexpr int directionIncrements[] = {
    1,   // WEST
    -1,  // EAST
    8,   // NORTH
    -8,  // SOUTH
    7,   // NORTHEAST
    -7,  // SOUTHWEST
    9,   // NORTHWEST
    -9   // SOUTHEAST
  };
} 

// get bit increment along certain direction
int getBitIncrement(int dir, bool attacking) {
  return (2 * attacking - 1 /* True -> 1, False -> -1 */) * directionIncrements[dir];
}

bool inBounds(int square) {
  return square < 64 && square >= 0;
}

bool Attack::isPawnAttackingSquare(Board* board, int king, Side attackingSide) {
  int col = king & 8;
  // bottom right to top left diagonal
  if (((col != 0) && (attackingSide == WHITE)) || ((col != 7) && (attackingSide == BLACK))) {
    int bitjmp = (attackingSide == WHITE) ? -9 : 9;
    if (inBounds(king+bitjmp) && (board->readBB(Pieces::type::PAWN, attackingSide) & (1ULL << (king+bitjmp)))) {
      return true;
    }
  }
  // bottom left to top right diagonal
  if (((col != 0) && (attackingSide == BLACK)) || ((col != 7) && (attackingSide == WHITE)) ) {
    int bitjmp = (attackingSide == WHITE) ? -7 : 7;
    if (inBounds(king+bitjmp) && (board->readBB(Pieces::type::PAWN, attackingSide) & (1ULL << (king+bitjmp)))) {
      return true;
    }
  }
  return false;
}

bool Attack::isKnightAttackingSquare(Board* board, int king, Side attackingSide) {
  return (board->knightAttacks()[king] & board->readBB(Pieces::type::KNIGHT, attackingSide)) != 0;  
}

bool Attack::isKingAttackingSquare(Board* board, int square, Side attackingSide) {
  return (board->kingAttacks()[square] & board->readBB(Pieces::type::KING, attackingSide)) != 0;
}

bool Attack::isSlidingPieceAttackingSquare(Board* board, int king, Side attackingSide) {
  auto queens = board->readBB(Pieces::type::QUEEN, attackingSide);
  auto rooks = board->readBB(Pieces::type::ROOK, attackingSide);
  auto bishops = board->readBB(Pieces::type::BISHOP, attackingSide);

  // updward lanes
  for (int d : {WEST, NORTHWEST, NORTH, NORTHEAST}) {
    auto blockingAllys = board->opposingBB(attackingSide) & board->slidingAttacks()[king][d];
    auto attackingOpps = board->allyBB(attackingSide) & board->slidingAttacks()[king][d];
    int ally_tz = __builtin_ctzll(blockingAllys), opp_tz = __builtin_ctzll(attackingOpps);
    if (ally_tz > opp_tz) {
      // opp piece can see king unobstructed
      if ((queens & (1ULL << opp_tz)) || ((Directions::isDiagonal(d) ? bishops : rooks) & (1ULL << opp_tz))) {
        return true;
      }
    }
  }

  // downward lanes
  for (int d : {EAST, SOUTHEAST, SOUTH, SOUTHWEST}) {
    auto blockingAllys = board->opposingBB(attackingSide) & board->slidingAttacks()[king][d];
    auto attackingOpps = board->allyBB(attackingSide) & board->slidingAttacks()[king][d];
    if (attackingOpps > blockingAllys) {
      // opp piece can see king unobstructed
      int opp_lz = __builtin_clzll(attackingOpps);
      if ((queens & (1ULL << (63 - opp_lz))) || ((Directions::isDiagonal(d) ? bishops : rooks) & (1ULL << (63 - opp_lz)))) {
        return true;
      }
    }
  }

  return false;
}

bool Attack::doesMovePutKingInCheck(Board* board, Move* move, Side kingSide) {
  auto simulatedBoard = *board;
  simulatedBoard.makeMove(move);
  int king = simulatedBoard.king(kingSide);
  assert(simulatedBoard.readBB(Pieces::type::KING, kingSide) == 1ULL << king);
  Side attackingSide = kingSide == WHITE ? BLACK : WHITE;
  return isPawnAttackingSquare(&simulatedBoard, king, attackingSide)
      || isKnightAttackingSquare(&simulatedBoard, king, attackingSide)
      || isSlidingPieceAttackingSquare(&simulatedBoard, king, attackingSide)
      || isKingAttackingSquare(&simulatedBoard, king, attackingSide);
}

bool Attack::doesMovePutOpponentKingInCheck(Board* board, Move* move, Side side) {
  return doesMovePutKingInCheck(board, move, side == WHITE ? BLACK : WHITE);
}

bool Attack::doesMoveExposeAllyKingToCheck(Board* board, Move* move, Side side) {
  return doesMovePutKingInCheck(board, move, side);
}

bool Attack::isSquareUnattacked(Board* board, int square, Side side) {
  auto move = Move(board->king(side), square, (side == WHITE) ? Pieces::piece::K : Pieces::piece::k);
  return !doesMovePutKingInCheck(board, &move, side);
}