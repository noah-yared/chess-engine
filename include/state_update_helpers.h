#pragma once

#include "board_state.h"
#include "move.h"

template<MoveType mType>
inline void updateEnpassantSquare(const Move<mType> move, BoardState& state) {
  state.setEnpassantSquare(std::nullopt); // no new enpassant square
}

template<>
inline void updateEnpassantSquare<MoveType::DoublePawnPush>(const Move<MoveType::DoublePawnPush> move, BoardState& state) {
  state.setEnpassantSquare(move.enpassantSquare());
}

template<MoveType mType>
inline void updateCastlingPrivs(const Move<mType> move, BoardState& state) {
  if (move.moved() == PieceType::KING)
    move.side() == Color::WHITE ? state.stripCastlingPrivileges<'K', 'Q'>() : state.stripCastlingPrivileges<'k', 'q'>();
  else if (move.moved() == PieceType::ROOK)
    move.side() == Color::WHITE ? state.stripCastlingPrivileges<'Q'>() : state.stripCastlingPrivileges<'q'>();
}

template<>
inline void updateCastlingPrivs<MoveType::Castle>(const Move<MoveType::Castle> move, BoardState& state) {
  move.side() == Color::WHITE ? state.stripCastlingPrivileges<'K', 'Q'>() : state.stripCastlingPrivileges<'k', 'q'>();
}

template<MoveType mType>
inline void updateTurn(const Move<mType> move, BoardState& state) {
  state.updateTurn();
}
