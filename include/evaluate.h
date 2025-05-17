#ifndef EVALUATE_H
#define EVALUATE_H

#include <array>

#include "sides.h"
#include "pieces.h"
#include "board.hpp"
#include "flags.h"
#include "Move.h"
#include "pieces.h"


namespace Evaluation{
  using Score = int;
  using Square = int;

  // Scoped enum for each side
  enum class Side {
    BLACK,
    WHITE
  };

  static inline bool isWhite(Side side) { return side == Side::WHITE; }
  static inline bool isBlack(Side side) { return side == Side::BLACK; }


  // Scoped enum for each piece type
  enum class PieceType {
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP, 
    QUEEN,
    KING
  };

  // Value for each piece type
  static constexpr std::array<Score, 6> PIECE_VALUES = {{
    100, // PAWN
    500, // ROOK
    320, // KNIGHT
    330, // BISHOP
    900, // QUEEN
    20000 // KING
  }};

  // mirror square for black
  static inline int mirrorSquare(Square square) {
    return 56 + (square & 7) - (square & 56);
  }

  // Get the type of piece on a given square for a given side
  static inline PieceType getPieceType(const Board& board, Square square, Side side) {
    for (PieceType piece : {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN})
      if (board.readBB(static_cast<Pieces::type>(piece), static_cast<::Side>(side)) & (1ULL << square)) 
        return piece;
    return PieceType::KING;
  }

  // Get the value of a piece for a given piece type
  static inline Score getMaterialValue(PieceType piece) {
    return PIECE_VALUES[static_cast<Pieces::type>(piece)];
  }


  // Piece-square tables (from WHITE's perspective)
  static inline const std::array<Score, 64> PIECE_SQUARES[6] = {
    // PAWN
    {{
      0,   0,   0,   0,   0,   0,   0,   0,
      50,  50,  50,  50,  50,  50,  50,  50,
      10,  10,  20,  30,  30,  20,  10,  10,
      5,   5,  10,  25,  25,  10,   5,   5,
      0,   0,   0,  20,  20,   0,   0,   0,
      5,  -5, -10,   0,   0, -10,  -5,   5,
      5,  10,  10, -20, -20,  10,  10,   5,
      0,   0,   0,   0,   0,   0,   0,   0
    }},

    // ROOK
    {{
      0,   0,   0,   0,   0,   0,   0,   0,
      5,  10,  10,  10,  10,  10,  10,   5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
     -5,   0,   0,   0,   0,   0,   0,  -5,
      0,   0,   0,   5,   5,   0,   0,   0
    }},

    // KNIGHT
    {{
      -50, -40, -30, -30, -30, -30, -40, -50,
      -40, -20,   0,   0,   0,   0, -20, -40,
      -30,   0,  10,  15,  15,  10,   0, -30,
      -30,   5,  15,  20,  20,  15,   5, -30,
      -30,   0,  15,  20,  20,  15,   0, -30,
      -30,   5,  10,  15,  15,  10,   5, -30,
      -40, -20,   0,   5,   5,   0, -20, -40,
      -50, -40, -30, -30, -30, -30, -40, -50
    }},

    // BISHOP
    {{
      -20, -10, -10, -10, -10, -10, -10, -20,
      -10,   0,   0,   0,   0,   0,   0, -10,
      -10,   0,   5,  10,  10,   5,   0, -10,
      -10,   5,   5,  10,  10,   5,   5, -10,
      -10,   0,  10,  10,  10,  10,   0, -10,
      -10,  10,  10,  10,  10,  10,  10, -10,
      -10,   5,   0,   0,   0,   0,   5, -10,
      -20, -10, -10, -10, -10, -10, -10, -20
    }},

    // QUEEN
    {{
      -20, -10, -10,  -5,  -5, -10, -10, -20,
      -10,   0,   0,   0,   0,   0,   0, -10,
      -10,   0,   5,   5,   5,   5,   0, -10,
       -5,   0,   5,   5,   5,   5,   0,  -5,
        0,   0,   5,   5,   5,   5,   0,  -5,
      -10,   5,   5,   5,   5,   5,   0, -10,
      -10,   0,   5,   0,   0,   0,   0, -10,
      -20, -10, -10,  -5,  -5, -10, -10, -20
    }},

    // KING
    {{
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -20, -30, -30, -40, -40, -30, -30, -20,
      -10, -20, -20, -20, -20, -20, -20, -10,
       20,  20,   0,   0,   0,   0,  20,  20,
       20,  30,  10,   0,   0,  10,  30,  20
    }}
  };

  // Get the value of a square for a given piece type, side, and square
  static inline Score getPositionalValue(PieceType piece, Square square, Side side) {
    Square lookupSquare = (side == Side::WHITE) ? mirrorSquare(square) : square;
    return PIECE_SQUARES[static_cast<Pieces::type>(piece)][lookupSquare];
  }

  class Evaluator {
    private:
      // static Score evaluateMaterial(const Board& board, Side side);
      static Score evaluateSide(const Board& board, Side side);
    public:
      static Score evaluate(const Board& board);
  };
}

#endif