#include "evaluate.h"

using namespace Evaluation;

using Bitboard = unsigned long long;

Score Evaluator::evaluate(const Board& board) {
  return evaluateSide(board, Side::WHITE) - evaluateSide(board, Side::BLACK);
}

Score Evaluator::evaluateSide(const Board& board, Side side) {
  Score score = 0;
  Bitboard bb = board.allyBB(static_cast<::Side>(side));
  for (Square square = 0; square < 64; ++square)
    if (bb & (1ULL << square)) {
      PieceType piece = getPieceType(board, square, side);
      score += getMaterialValue(piece) + getPositionalValue(piece, square, side);
    }
  return score;
}

/* 
 * NOT USED -- USING A SINGLE PASS TO EVALUATE BOTH MATERIAL AND POSITION 
 * MAY REFACTOR AND SEPARATE FUNCTIONS IF I UPDATE EVALUATION FUNCTIONS
 */
// Score Evaluator::evaluateMaterial(const Board& board, Side side) {
//   ull bitmask = 1ULL;
//   Bitboard bb = board.allyBB(side);
//   Score score = 0;
//   for (Square square = 0; square < 64; ++square)
//     if (bb & bitmask)
//       score += getMaterialValue(getPieceType(board, square, side));
//     bitmask <<= 1;
//   return score;
// }

// Score Evaluator::evaluatePosition(const Board& board, Side side) {
//   Score score = 0;
//   Bitboard bb = board.allyBB(side);
//   for (Square square = 0; square < 64; ++square)
//     if (bb & (1ULL << square))
//       score += getPositionalValue(getPieceType(board, square, side), square, side);
//   return score;
// }
