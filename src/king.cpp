#include "king.h"
#include "kingSafety.h"
#include "Move.h"
#include "utils.h"

#include <memory>

namespace CastlingMasks {
static const ull BLACK_QUEENSIDE_CASTLE_MASK = 0x70ULL << 56;
static const ull BLACK_KINGSIDE_CASTLE_MASK = 0x06ULL << 56;
static const ull WHITE_QUEENSIDE_CASTLE_MASK = 0x70ULL;
static const ull WHITE_KINGSIDE_CASTLE_MASK = 0x06ULL;
}

std::vector<std::unique_ptr<Move>> King::generateMoves(Board *board, Side side) {
  int king = board->king(side);
  ull attacks = board->kingAttacks()[king] & ~board->allyBB(side);
  // attacks ^= board->readBB(Pieces::KING, side); /* remove king */

  std::vector<std::unique_ptr<Move>> kingMoves;

  auto piece_t = side ? Pieces::K : Pieces::k;
  while (attacks) {
    int square = __builtin_ctzll(attacks);
    Move move(king, square, piece_t);
    checkAndSetCapture(board, &move, side);
    if (Attack::isSquareUnattacked(board, square, side)) {
      // safe square for king so add move
      kingMoves.emplace_back(std::make_unique<Move>(move));
    }
    attacks &= (attacks - 1);
  }

  ull combinedBB = board->readCombinedBB();
  // handle castling moves
  for (auto square : board->availableCastlingDestinations(side)) {
    bool queenSide = square > king;
    // mask for checking that no pieces between king and rook
    ull intermediateBitmask = queenSide ? (side == WHITE ? CastlingMasks::WHITE_QUEENSIDE_CASTLE_MASK : CastlingMasks::BLACK_QUEENSIDE_CASTLE_MASK) 
                            : (side == WHITE ? CastlingMasks::WHITE_KINGSIDE_CASTLE_MASK : CastlingMasks::BLACK_KINGSIDE_CASTLE_MASK);
    // skip if piece between king and rook
    if (combinedBB & intermediateBitmask) continue;
    // make sure no pieces attacking squares between rook and king
    if (  Attack::isSquareUnattacked(board, king, side)
       && Attack::isSquareUnattacked(board, (king + square) / 2, side)
       && Attack::isSquareUnattacked(board, square, side)) 
      {
        Move move(king, square, piece_t, Flags::CASTLE);
        kingMoves.emplace_back(std::make_unique<Move>(move));
      }
  }
  return kingMoves;
}