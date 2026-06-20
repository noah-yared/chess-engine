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

// Get the value of a piece for a given piece type
inline int getMaterialValue(PieceType piece) noexcept
{
    return PIECE_VALUES[static_cast<int>(piece)];
}
inline int getMaterialValue(int pieceKey) noexcept { return PIECE_VALUES[pieceKey]; }

// Get the value of a square for a given piece type, side, and square
inline int getPositionalValue(PieceType pType, int square, Color color) noexcept
{
    int lookupSquare = (color == Color::WHITE) ? mirrorSquare(square) : square;
    return PIECE_SQUARES[static_cast<int>(pType)][lookupSquare];
}
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

int Evaluator::evaluate_v1(const Bitboards& bitboards) noexcept
{
    int eval = evaluateSide_v1(bitboards, Color::WHITE) - evaluateSide_v1(bitboards, Color::BLACK);
    return std::clamp(eval, MIN_EVAL, MAX_EVAL);
}

int Evaluator::evaluate_v2(const Bitboards& bitboards) noexcept
{
    int eval = evaluateSide_v2(bitboards, Color::WHITE) - evaluateSide_v2(bitboards, Color::BLACK);
    return std::clamp(eval, MIN_EVAL, MAX_EVAL);
}

int Evaluator::evaluate(const Bitboards& bitboards) noexcept
{
    int eval = evaluateSide<Color::WHITE>(bitboards) - evaluateSide<Color::BLACK>(bitboards);
    return std::clamp(eval, MIN_EVAL, MAX_EVAL);
}

int Evaluator::evaluateSide_v1(const Bitboards& bitboards, Color color) noexcept
{
    int score = 0;
    u64 bitmask = 1ULL, bb = bitboards.allyBB(color);
    for (int square = 0; square < 64; ++square, bitmask <<= 1)
        if (bb & bitmask)
        {
            PieceType pType = bitboards.getPieceType(square, color);
            score += getMaterialValue(pType) + getPositionalValue(pType, square, color);
        }
    return score;
}

int Evaluator::evaluateSide_v2(const Bitboards& bitboards, Color color) noexcept
{
    return BitUtils::accumulateBits(
        bitboards.allyBB(color),
        [&, color](int score, int square) noexcept
        {
            PieceType pType = bitboards.getPieceType(square, color);
            return score + getMaterialValue(pType) + getPositionalValue(pType, square, color);
        },
        0);
}

template <Color color>
int Evaluator::evaluateSide(const Bitboards& bitboards) noexcept
{
    struct AccType
    {
        int pKey = 0, score = 0;
    };
    auto start = color == Color::WHITE ? bitboards.wStart() : bitboards.bStart(),
         end = color == Color::WHITE ? bitboards.wEnd() : bitboards.bEnd();
    return std::accumulate(start, end, AccType{},
                           [&bitboards](AccType acc, u64 bb) noexcept -> AccType
                           {
                               return {acc.pKey + 1,
                                       BitUtils::accumulateBits<int>(
                                           bb,
                                           [pKey = acc.pKey](int score, int square) noexcept
                                           {
                                               return score + getMaterialValue(pKey) +
                                                      getPositionalValue<color>(pKey, square);
                                           },
                                           acc.score)};
                           })
        .score;
}
