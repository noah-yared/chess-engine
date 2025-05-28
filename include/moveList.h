#pragma once

#include <vector>
#include <algorithm>
#include <span>

#include "Move.h"

static const int MAX_MOVES = 218;

// Value for each piece type
static constexpr std::array<int, 6> PIECE_VALUES = {{
  100, // PAWN
  500, // ROOK
  320, // KNIGHT
  330, // BISHOP
  900, // QUEEN
  20000 // KING
}};

class MoveList {
  Move moves[MAX_MOVES];
  int length;

  inline int pieceValue(Pieces::piece piece) {
    return PIECE_VALUES[(piece - 1) % 6];
  }

  inline int value(Move move) {
    if ( ! move.isCapture()) return 0;
    return pieceValue(move.capturedPiece()) - pieceValue(move.p());
  }

 public: 
  MoveList(): length(0) {};

  inline void clear() { length = 0; }

  inline void push(Move move) { moves[length++] = move; }

  void sort() {
    // piece = moved + start + end + capture + flags
    auto comp = [this](Move& a, Move& b) { return value(a) > value(b); };
    std::sort(moves, moves + length, comp);
  }

  std::span<const Move> getMoves() {
    return std::span<const Move>(moves, length);
  }
};