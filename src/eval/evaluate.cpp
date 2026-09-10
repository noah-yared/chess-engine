#include "eval/evaluate.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "board/bitboards.h"
#include "board/constants.h"
#include "board/pieces.h"
#include "util/bit_utils.h"
#include "util/platform.h"

namespace
{

// Value for each piece type
inline constexpr std::array<int, 6> PIECE_VALUES = {{
    100,  // PAWN
    500,  // ROOK
    320,  // KNIGHT
    330,  // BISHOP
    900,  // QUEEN
    20000 // KING
}};

// Piece-square tables (from WHITE's perspective)
inline constexpr std::array<std::array<int, 64>, 6> PIECE_SQUARES = {{
    // PAWN
    {{0,  0,   0,  0, 0,  0,  0,  0,   50,  50, 50, 50, 50, 50, 50, 50, 10, 10, 20, 30, 30,  20,
      10, 10,  5,  5, 10, 25, 25, 10,  5,   5,  0,  0,  0,  20, 20, 0,  0,  0,  5,  -5, -10, 0,
      0,  -10, -5, 5, 5,  10, 10, -20, -20, 10, 10, 5,  0,  0,  0,  0,  0,  0,  0,  0}},

    // ROOK
    {{0, 0,  0,  0,  0,  0, 0, 0, 5, 10, 10, 10, 10, 10, 10, 5, -5, 0,  0,  0, 0, 0,
      0, -5, -5, 0,  0,  0, 0, 0, 0, -5, -5, 0,  0,  0,  0,  0, 0,  -5, -5, 0, 0, 0,
      0, 0,  0,  -5, -5, 0, 0, 0, 0, 0,  0,  -5, 0,  0,  0,  5, 5,  0,  0,  0}},

    // KNIGHT
    {{-50, -40, -30, -30, -30, -30, -40, -50, -40, -20, 0,   0,   0,   0,   -20, -40,
      -30, 0,   10,  15,  15,  10,  0,   -30, -30, 5,   15,  20,  20,  15,  5,   -30,
      -30, 0,   15,  20,  20,  15,  0,   -30, -30, 5,   10,  15,  15,  10,  5,   -30,
      -40, -20, 0,   5,   5,   0,   -20, -40, -50, -40, -30, -30, -30, -30, -40, -50}},

    // BISHOP
    {{-20, -10, -10, -10, -10, -10, -10, -20, -10, 0,   0,   0,   0,   0,   0,   -10,
      -10, 0,   5,   10,  10,  5,   0,   -10, -10, 5,   5,   10,  10,  5,   5,   -10,
      -10, 0,   10,  10,  10,  10,  0,   -10, -10, 10,  10,  10,  10,  10,  10,  -10,
      -10, 5,   0,   0,   0,   0,   5,   -10, -20, -10, -10, -10, -10, -10, -10, -20}},

    // QUEEN
    {{-20, -10, -10, -5, -5, -10, -10, -20, -10, 0,   0,   0,  0,  0,   0,   -10,
      -10, 0,   5,   5,  5,  5,   0,   -10, -5,  0,   5,   5,  5,  5,   0,   -5,
      0,   0,   5,   5,  5,  5,   0,   -5,  -10, 5,   5,   5,  5,  5,   0,   -10,
      -10, 0,   5,   0,  0,  0,   0,   -10, -20, -10, -10, -5, -5, -10, -10, -20}},

    // KING
    {{-30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30, -30, -40, -40, -50, -50, -40, -40, -30,
      -20, -30, -30, -40, -40, -30, -30, -20, -10, -20, -20, -20, -20, -20, -20, -10,
      20,  20,  0,   0,   0,   0,   20,  20,  20,  30,  10,  0,   0,   10,  30,  20}},
}};

// mirror square for black
inline int mirrorSquare(int square) noexcept { return 56 + (square & 7) - (square & 56); }

inline int getMaterialValue(int pieceKey) noexcept { return PIECE_VALUES[pieceKey]; }

template <Color color>
inline int getPositionalValue(int pieceKey, int square) noexcept
{
    if constexpr (color == Color::WHITE)
    {
        return PIECE_SQUARES[pieceKey][mirrorSquare(square)];
    }
    else
    {
        return PIECE_SQUARES[pieceKey][square];
    }
}
} // unnamed namespace

int Evaluator::evaluate(const Bitboards& bitboards) noexcept
{
    int eval = evaluateSide<Color::WHITE>(bitboards) - evaluateSide<Color::BLACK>(bitboards);
    return std::clamp(eval, MIN_EVAL, MAX_EVAL);
}

template <Color color>
int Evaluator::evaluateSide(const Bitboards& bitboards) noexcept
{
    auto start = color == Color::WHITE ? bitboards.wStart() : bitboards.bStart();
    auto end = color == Color::WHITE ? bitboards.wEnd() : bitboards.bEnd();
    return std::accumulate(start, end, 0,
                           [i = 0](int score, u64 bb) mutable noexcept
                           {
                               return BitUtils::accumulateBits<int>(
                                   bb,
                                   [pKey = i++](int score, int square) noexcept
                                   {
                                       return score + getMaterialValue(pKey) +
                                              getPositionalValue<color>(pKey, square);
                                   },
                                   score);
                           });
}
