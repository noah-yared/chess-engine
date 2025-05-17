#include "sliding.h"

#include <iterator>
#include <memory>
#include <bitset>
#include <vector>

#include "board.hpp"
#include "Move.h"
#include "directions.h"
#include "kingSafety.h"
#include "pieces.h"
#include "sides.h"

typedef unsigned long long ull;

std::vector<std::unique_ptr<Move>> Sliding::getMovesAlongDirection(
    Board* board, Side side, int square, Pieces::piece piece,
    int dir) {
  std::vector<std::unique_ptr<Move>> moves;

  ull reachableSquares;
  int bitcnt, bitinc = getBitIncrement(dir, true);

  reachableSquares = board->slidingAttacks()[square][dir]; // all squares in direction

  int blockingSquare;
  ull blockingSquares = reachableSquares & board->readCombinedBB(); // occupied squares
  
  if (blockingSquares) {
    blockingSquare = (dir & 1) ? 63 - __builtin_clzll(blockingSquares) : __builtin_ctzll(blockingSquares);
    if (board->allyBB(side) & (1ULL << blockingSquare)) { // ally is blocking path
      blockingSquare -= bitinc; // cannot move onto ally so move back one square
    }
    bitcnt = abs((blockingSquare - square) / bitinc);
  } else { // no blocking squares
    bitcnt = __builtin_popcountll(reachableSquares);
  }

  for (int i = 1; i <= bitcnt; i++) {
    int finalSquare = square + bitinc * i;
    Move move(square, finalSquare, piece);
    if (!Attack::doesMoveExposeAllyKingToCheck(board, &move, side)) {
      if (Attack::doesMovePutOpponentKingInCheck(board, &move, side))
        move.setFlag(Flags::CHECK);
      moves.emplace_back(std::make_unique<Move>(move));
    }
  }
  return moves;
}

std::vector<std::unique_ptr<Move>> Rook::generateMoves(Board* board,
                                                        Side side) {
  std::vector<std::unique_ptr<Move>> rookMoves;

  ull rookBB = board->readBB(side ? Pieces::R : Pieces::r);
  while (rookBB) {
    int square = __builtin_ctzll(rookBB);
    for (int direction :
         {NORTH, SOUTH, EAST, WEST}) {
      auto movesAlongDirection = Sliding::getMovesAlongDirection(
          board, side, square, side ? Pieces::R : Pieces::r, direction);
      std::move(movesAlongDirection.begin(), movesAlongDirection.end(),
                std::back_inserter(rookMoves));
    }
    rookBB &= (rookBB - 1);
  }
  return rookMoves;
}

std::vector<std::unique_ptr<Move>> Bishop::generateMoves(Board* board,
                                                          Side side) {
  std::vector<std::unique_ptr<Move>> bishopMoves;

  ull bishopBB = board->readBB(side ? Pieces::B : Pieces::b);
  while (bishopBB) {
    int square = __builtin_ctzll(bishopBB);
    for (int direction :
         {NORTHEAST, SOUTHWEST, NORTHWEST, SOUTHEAST}) {
      // handle directions
      auto movesAlongDirection = Sliding::getMovesAlongDirection(
          board, side, square, side ? Pieces::B : Pieces::b, direction);
      std::move(movesAlongDirection.begin(), movesAlongDirection.end(),
                std::back_inserter(bishopMoves));
    }
    bishopBB &= (bishopBB - 1);
  }
  return bishopMoves;
}

std::vector<std::unique_ptr<Move>> Queen::generateMoves(Board* board,
                                                        Side side) {
  std::vector<std::unique_ptr<Move>> queenMoves;

  ull queenBB = board->readBB(side ? Pieces::Q : Pieces::q);
  while (queenBB) {
    int square = __builtin_ctzll(queenBB);
    for (int direction :
         {NORTH, NORTHEAST, SOUTH, SOUTHWEST,
          EAST, NORTHWEST, WEST, SOUTHEAST}) {
      // handle directions
      auto movesAlongDirection = Sliding::getMovesAlongDirection(
          board, side, square, side ? Pieces::Q : Pieces::q, direction);
      std::move(movesAlongDirection.begin(), movesAlongDirection.end(),
                std::back_inserter(queenMoves));
    }
    queenBB &= (queenBB - 1);
  }
  return queenMoves;
}