#include "king.h"
#include "kingSafety.h"
#include "Move.h"
#include "utils.h"

#include <memory>

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
  // handle castling moves
  for (auto square : board->availableCastlingDestinations(side)) {
    bool queenSide = square > king;
    ull bitmask = queenSide ? 0x7000000000000070ULL : 0x600000000000006ULL;
    int lane = queenSide ? WEST : EAST;
    if (board->slidingAttacks()[king][lane] & board->readCombinedBB() & bitmask) continue;
    if (Attack::isSquareUnattacked(board, king + 2 * (queenSide - 1), side)
        && Attack::isSquareUnattacked(board, square, side)) { // free to castle
          Move move(king, square, piece_t, Flags::CASTLE);
          kingMoves.emplace_back(std::make_unique<Move>(move));
        }
  }
  return kingMoves;
}