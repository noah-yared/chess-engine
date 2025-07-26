#include "evaluate.h"

#include <cstdint>
#include <limits>

#include "bitboards.h"
#include "constants.h"
#include "pieces.h"
#include "platform.h"

namespace {

// Value for each piece type
inline constexpr std::array<int, 6> PIECE_VALUES = {{
  100, // PAWN
  500, // ROOK
  320, // KNIGHT
  330, // BISHOP
  900, // QUEEN
  20000 // KING
}};

// Piece-square tables (from WHITE's perspective)
inline constexpr std::array<std::array<int, 64>, 6> PIECE_SQUARES = {{
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
  }},
}};

// mirror square for black
inline int mirrorSquare(int square) {
  return 56 + (square & 7) - (square & 56);
}

// Get the value of a piece for a given piece type
inline int getMaterialValue(PieceType piece) {
  return PIECE_VALUES[static_cast<int>(piece)];
}

// Get the value of a square for a given piece type, side, and square
inline int getPositionalValue(PieceType pType, int square, Color color) {
  int lookupSquare = (color == Color::WHITE) ? mirrorSquare(square) : square;
  return PIECE_SQUARES[static_cast<int>(pType)][lookupSquare];
}
} // unnamed namespace

int Evaluator::evaluate(const Bitboards& bitboards) {
  int eval = evaluateSide(bitboards, Color::WHITE) - evaluateSide(bitboards, Color::BLACK);
  return std::max(MIN_EVAL, std::min(MAX_EVAL, eval));
}

int Evaluator::evaluateSide(const Bitboards& bitboards, Color color) {
  int score = 0;
  u64 bitmask = 1ULL, bb = bitboards.allyBB(color);
  for (int square = 0; square < 64; ++square, bitmask <<= 1)
    if (bb & bitmask) {
      PieceType pType = bitboards.getPieceType(square, color);
      score += getMaterialValue(pType) + getPositionalValue(pType, square, color);
    }
  return score;
}

/* 
 * NOT USED -- USING A SINGLE PASS TO EVALUATE BOTH MATERIAL AND POSITION 
 * MAY REFACTOR AND SEPARATE FUNCTIONS IF I UPDATE EVALUATION FUNCTIONS
 */
// Score Evaluator::evaluateMaterial(const Bitboards& bitboards, Side side) {
//   ull bitmask = 1ULL;
//   Bitboards bb = bitboards.allyBB(side);
//   Score score = 0;
//   for (Square square = 0; square < 64; ++square)
//     if (bb & bitmask)
//       score += getMaterialValue(getPieceType(bitboards, square, side));
//     bitmask <<= 1;
//   return score;
// }

// Score Evaluator::evaluatePosition(const Bitboards& bitboards, Side side) {
//   Score score = 0;
//   Bitboards bb = bitboards.allyBB(side);
//   for (Square square = 0; square < 64; ++square)
//     if (bb & (1ULL << square))
//       score += getPositionalValue(getPieceType(bitboards, square, side), square, side);
//   return score;
// }
