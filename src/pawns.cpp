#include "pawns.h"

#include <memory>
#include <vector>
#include <optional>
#include "board.hpp"
#include "Move.h"
#include "kingSafety.h"
#include "pieces.h"
#include "sides.h"

// #define DEBUG

// bitmask to get important ranks/files
ull firstFileBitmask  = 0x8080808080808080ULL;
ull middleFilesBitmask = 0x7E7E7E7E7E7E7E7EULL;
ull lastFileBitmask = 0x0101010101010101ULL;
ull secondRankBitmask = 0x00FF000000000000ULL;
ull seventhRankBitmask = 0x000000000000FF00ULL;
ull firstRankBitmask = 0xFF00000000000000ULL;
ull lastRankBitmask  = 0xFFULL;


void printBmap(ull bitmap) {
  ull mask = 0xFFULL;
  for (int row = 0; row < 8; row++) {
    std::cout << std::bitset<8>((bitmap >> (56-8*row)) & mask).to_string() << '\n';
  }
}

std::vector<std::unique_ptr<Move>> Pawn::getDoubleSteps(Board *board,
                                                        Side side) {
  std::vector<std::unique_ptr<Move>> doubleSteps;
  const ull emptySquares = ~board->readCombinedBB();
  const ull bitmask = (side == WHITE) ? seventhRankBitmask : secondRankBitmask;
  ull candidatePawnsBB =
      board->readBB((Pieces::piece)(side ? Pieces::P : Pieces::p)) & bitmask;
  if (!candidatePawnsBB) return doubleSteps;
  candidatePawnsBB = emptySquares & ((side == WHITE) ? candidatePawnsBB << 8 : candidatePawnsBB >> 8);
  candidatePawnsBB = emptySquares & ((side == WHITE) ? candidatePawnsBB << 8 : candidatePawnsBB >> 8);
  int sft = 8 * (2 * (side == WHITE) - 1);
  while (candidatePawnsBB) {
    int square = __builtin_ctzll(candidatePawnsBB);
    Move move(square - 2 * sft, square, 
              (side == WHITE) ? Pieces::P : Pieces::p,
              Flags::DOUBLESTEP);
    if (!doesMoveExposeAllyKingToCheck(board, &move, side)) {
      if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
        move.setFlag(Flags::CHECK);
      }
      doubleSteps.emplace_back(std::make_unique<Move>(move));
    }
    candidatePawnsBB &= (candidatePawnsBB - 1);
  }
  return doubleSteps;  // vector of unique_ptrs that point to move
}

std::vector<std::unique_ptr<Move>> Pawn::getSingleSteps(Board *board,
                                                        Side side) {
  std::vector<std::unique_ptr<Move>> singleSteps;
  const ull emptySquares = ~board->readCombinedBB();
  ull candidatePawnsBB =
      board->readBB((Pieces::piece)(side ? Pieces::P : Pieces::p));
  candidatePawnsBB = ((side == WHITE) ? candidatePawnsBB << 8 : candidatePawnsBB >> 8) & emptySquares;   
  int sft = 8 * (2 * (side == WHITE) - 1);
  while (candidatePawnsBB) {
    int square = __builtin_ctzll(candidatePawnsBB);
    bool isPromoting = (square & 56) == 0 || (square & 56) == 56;
    Move move(square - sft, square,
              (side == WHITE) ? Pieces::P : Pieces::p,
              (isPromoting ? Flags::PROMOTION : Flags::NONE));
    if (!doesMoveExposeAllyKingToCheck(board, &move, side)) {
      if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
        move.setFlag(Flags::CHECK);
      }
      singleSteps.emplace_back(std::make_unique<Move>(move));
    }
    candidatePawnsBB &= (candidatePawnsBB - 1);
  }
  return singleSteps;  // vector of unique_ptrs that point to move
}

std::vector<std::unique_ptr<Move>> Pawn::getDiagonalAttacks(Board *board,
                                                            Side side) {
  std::vector<std::unique_ptr<Move>> attackingMoves;
  ull candidatePawnsBB =
      board->readBB((Pieces::piece)(side ? Pieces::P : Pieces::p));

  // compile first file attacks
  ull firstFilePawnsBB = candidatePawnsBB & firstFileBitmask;
  ull firstFileAttacks = side == WHITE ? firstFilePawnsBB << 7 : firstFilePawnsBB >> 9;

  // compile middle files attacks
  ull middleFilesPawnsBB = candidatePawnsBB & middleFilesBitmask;
  ull middleFileAttacks = side == WHITE ? (middleFilesPawnsBB << 7) | (middleFilesPawnsBB << 9)
                                        : (middleFilesPawnsBB >> 7) | (middleFilesPawnsBB >> 9);

  // compile last file attacks
  ull lastFilePawnsBB = candidatePawnsBB & lastFileBitmask;
  ull lastFileAttacks = side == WHITE ? lastFilePawnsBB << 9 : lastFilePawnsBB >> 7;


  ull compiledPawnAttacks =
      (firstFileAttacks | middleFileAttacks | lastFileAttacks) & (board->opposingBB(side));

  // int n = __builtin_popcountll(compiledPawnAttacks);
  int sign = 2 * (side == WHITE) - 1;
  int lAtt = sign * (8 + sign), rAtt = sign * (8 - sign);
  while (compiledPawnAttacks) {
    int square = __builtin_ctzll(compiledPawnAttacks);
    bool isPromoting = (square & 56) == 56 || (square & 56) == 0;
    bool isLeftAttacking = (square & 7) != 0;
    bool isRightAttacking = (square & 7) != 7;
    if (isLeftAttacking && (candidatePawnsBB & (1ULL << (square - lAtt)))) {
      // left pawn attack
      Move move(square-lAtt, square, side ? Pieces::P : Pieces::p, 
                (isPromoting ? Flags::PROMOTION : Flags::NONE));
      if (!doesMoveExposeAllyKingToCheck(board, &move, side)) {
        if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
          move.setFlag(Flags::CHECK);
        }
        attackingMoves.emplace_back(std::make_unique<Move>(move));
      }
    }
    if (isRightAttacking && (candidatePawnsBB & (1ULL << (square - rAtt)))) {
      // right pawn attack
      Move move(square-rAtt, square, side ? Pieces::P : Pieces::p, 
                ((square & 56) == 0 || (square & 56) == 0) ? Flags::PROMOTION : Flags::NONE);
      if (!doesMoveExposeAllyKingToCheck(board, &move, side)) {
        if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
          move.setFlag(Flags::CHECK);
        }
        attackingMoves.emplace_back(std::make_unique<Move>(move));
      } 
    }
    compiledPawnAttacks &= (compiledPawnAttacks - 1);
  }
  return attackingMoves;
}

std::optional<std::pair<std::unique_ptr<Move>, std::unique_ptr<Move>>> Pawn::getEnpassantMoves(Board *board, Side side) {
  std::optional<std::pair<std::unique_ptr<Move>, std::unique_ptr<Move>>> enpassantMoves = std::nullopt;
  int enpassantSquare = board->getEnpassantSquare();
  if (enpassantSquare == -1) return enpassantMoves;
  ull candidatePawnsBB = board->readBB((Pieces::piece)(side ? Pieces::P : Pieces::p));
  int sign = 2 * (side == WHITE) - 1;
  int lAtt = sign * (8 + sign), rAtt = sign * (8 - sign);
  bool isLeftAttacking = (enpassantSquare & 7) != 0;
  bool isRightAttacking = (enpassantSquare & 7) != 7;
  if (isLeftAttacking && (candidatePawnsBB & (1ULL << (enpassantSquare - lAtt)))) {
    // left pawn attack
    Move move(enpassantSquare-lAtt, enpassantSquare, side ? Pieces::P : Pieces::p, Flags::ENPASSANT);
    if (!doesMoveExposeAllyKingToCheck(board, &move, side)){
      if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
        move.setFlag(Flags::CHECK);
      }
      enpassantMoves.emplace(std::make_pair(std::make_unique<Move>(move), nullptr));
    }
  }
  if (isRightAttacking && (candidatePawnsBB & (1ULL << (enpassantSquare - rAtt)))) {
    // right pawn attack
    Move move(enpassantSquare-rAtt, enpassantSquare, side ? Pieces::P : Pieces::p, Flags::ENPASSANT);
    if (!doesMoveExposeAllyKingToCheck(board, &move, side)) {
      if (enpassantMoves.has_value()) {
        if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
          move.setFlag(Flags::CHECK);
        }
        enpassantMoves->second = std::make_unique<Move>(move);
      } else {
        if (Attack::doesMovePutOpponentKingInCheck(board, &move, side)) {
          move.setFlag(Flags::CHECK);
        }
        enpassantMoves.emplace(std::make_pair(std::make_unique<Move>(move), nullptr));
      }
    }
  }

  return enpassantMoves;
}

// inefficient implementation, will optimize after testing/profiling
std::vector<std::unique_ptr<Move>> Pawn::generateMoves(Board* board, Side side) {
  std::vector<std::unique_ptr<Move>> compiledMoves;

  #ifdef DEBUG
  // printBmap(board->readBB((Pieces::piece)(side ? Pieces::P : Pieces::p)));
  #endif

  for (auto&& move_uptr : Pawn::getDiagonalAttacks(board, side))
    compiledMoves.emplace_back(std::move(move_uptr));

  for (auto&& move_uptr : Pawn::getDoubleSteps(board, side))
    compiledMoves.emplace_back(std::move(move_uptr));
  
  for (auto&& move_uptr : Pawn::getSingleSteps(board, side))
    compiledMoves.emplace_back(std::move(move_uptr));

  auto enpassantMoves = Pawn::getEnpassantMoves(board, side);
  if (enpassantMoves.has_value()) {
    compiledMoves.emplace_back(std::move(enpassantMoves->first));
    if (enpassantMoves->second)
      compiledMoves.emplace_back(std::move(enpassantMoves->second));
  }

  return compiledMoves;
}

