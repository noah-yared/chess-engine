#include <algorithm>
#include <memory>
#include <vector>

#include "board.hpp"
#include "movegen.h"
#include "pawns.h"
#include "knights.h"
#include "king.h"
#include "sliding.h"

std::vector<std::unique_ptr<Move>> generateMoves(Board* board, Side side) {
  std::vector<std::unique_ptr<Move>> moves;
  // reserve space for max possible moves for given side of board (218)
  moves.reserve(218);

  auto pawnMoves = Pawn::generateMoves(board, side), rookMoves = Rook::generateMoves(board, side), knightMoves = Knight::generateMoves(board, side);
  auto bishopMoves = Bishop::generateMoves(board, side), queenMoves = Queen::generateMoves(board, side), kingMoves = King::generateMoves(board, side);

  moves.insert(moves.end(), std::make_move_iterator(pawnMoves.begin()), std::make_move_iterator(pawnMoves.end()));
  moves.insert(moves.end(), std::make_move_iterator(bishopMoves.begin()), std::make_move_iterator(bishopMoves.end()));
  moves.insert(moves.end(), std::make_move_iterator(rookMoves.begin()), std::make_move_iterator(rookMoves.end()));
  moves.insert(moves.end(), std::make_move_iterator(knightMoves.begin()), std::make_move_iterator(knightMoves.end()));
  moves.insert(moves.end(), std::make_move_iterator(queenMoves.begin()), std::make_move_iterator(queenMoves.end()));
  moves.insert(moves.end(), std::make_move_iterator(kingMoves.begin()), std::make_move_iterator(kingMoves.end()));

  return moves;
}