#include "evaluate.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "bitboards.h"
#include "bit_utils.h"
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
inline int mirrorSquare(int square) noexcept {
  return 56 + (square & 7) - (square & 56);
}

// Get the value of a piece for a given piece type
inline int getMaterialValue(PieceType piece) noexcept {
  return PIECE_VALUES[static_cast<int>(piece)];
}
inline int getMaterialValue(int pieceKey) noexcept {
  return PIECE_VALUES[pieceKey];
}

// Get the value of a square for a given piece type, side, and square
inline int getPositionalValue(PieceType pType, int square, Color color) noexcept {
  int lookupSquare = (color == Color::WHITE) ? mirrorSquare(square) : square;
  return PIECE_SQUARES[static_cast<int>(pType)][lookupSquare];
}
template<Color color>
inline int getPositionalValue(int pieceKey, int square) noexcept {
  if constexpr (color == Color::WHITE) {
    return PIECE_SQUARES[pieceKey][mirrorSquare(square)];
  } else {
    return PIECE_SQUARES[pieceKey][square];
  }
}
} // unnamed namespace

int Evaluator::evaluate_v1(const Bitboards& bitboards) noexcept {
  int eval = evaluateSide_v1(bitboards, Color::WHITE) - evaluateSide_v1(bitboards, Color::BLACK);
  return std::max(MIN_EVAL, std::min(MAX_EVAL, eval));
}

int Evaluator::evaluate_v2(const Bitboards& bitboards) noexcept {
  int eval = evaluateSide_v2(bitboards, Color::WHITE) - evaluateSide_v2(bitboards, Color::BLACK);
  return std::max(MIN_EVAL, std::min(MAX_EVAL, eval));
}

int Evaluator::evaluate_v3(const Bitboards& bitboards) noexcept {
  int eval = evaluateSide_v3(bitboards, Color::WHITE) - evaluateSide_v3(bitboards, Color::BLACK);
  return std::max(MIN_EVAL, std::min(MAX_EVAL, eval));
}

int Evaluator::evaluateSide_v1(const Bitboards& bitboards, Color color) noexcept {
  int score = 0;
  u64 bitmask = 1ULL, bb = bitboards.allyBB(color);
  for (int square = 0; square < 64; ++square, bitmask <<= 1)
    if (bb & bitmask) {
      PieceType pType = bitboards.getPieceType(square, color);
      score += getMaterialValue(pType) + getPositionalValue(pType, square, color);
    }
  return score;
}

int Evaluator::evaluateSide_v2(const Bitboards& bitboards, Color color) noexcept {
  return BitUtils::accumulateBits(bitboards.allyBB(color), [&, color](int score, int square) noexcept { 
    PieceType pType = bitboards.getPieceType(square, color);
    return score + getMaterialValue(pType) + getPositionalValue(pType, square, color);
  }, 0);
}

int Evaluator::evaluateSide_v3(const Bitboards& bitboards, Color color) noexcept {
  struct AccType { int pKey = 0, score = 0; };
  if (color == Color::WHITE) {
    return std::accumulate(bitboards.wStart(), bitboards.wEnd(), AccType{}, [](AccType acc, u64 bb) noexcept -> AccType {
      return {
        acc.pKey + 1, BitUtils::accumulateBits<int>(bb, [pKey=acc.pKey](int score, int square) noexcept {
          return score + getMaterialValue(pKey) + getPositionalValue<Color::WHITE>(pKey, square);
        }, acc.score)
      };
    }).score;
  } else {
    return std::accumulate(bitboards.bStart(), bitboards.bEnd(), AccType{}, [](AccType acc, u64 bb) noexcept -> AccType {
      return {
        acc.pKey + 1, BitUtils::accumulateBits<int>(bb, [pKey=acc.pKey](int score, int square) noexcept {
          return score + getMaterialValue(pKey) + getPositionalValue<Color::BLACK>(pKey, square);
        }, acc.score)
      };
    }).score;
  }
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
