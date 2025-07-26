#pragma once

#include <cassert>
#include <optional>
#include <vector>

#include "bitboards.h"
#include "board_state.h"
#include "delta.h"
#include "move.h"
#include "pieces.h"

template<MoveType mType>
inline std::vector<Delta> pieceSquareDeltas(const Move<mType> move) { return {}; }

template<>
inline std::vector<Delta> pieceSquareDeltas<MoveType::Normal>(const Move<MoveType::Normal> move) {
  std::vector<Delta> deltas = {
    Delta::Remove( move.movedKey(), move.start() ),
    Delta::Place ( move.movedKey(), move.end()   ),
  };
  if (auto maybeCapturedKey = move.capturedKey(); maybeCapturedKey) {
    deltas.push_back(Delta::Remove( *maybeCapturedKey, move.end() ));
  }
  return deltas;
}

template<>
inline std::vector<Delta> pieceSquareDeltas<MoveType::Enpassant>(const Move<MoveType::Enpassant> move) {
  return {
    Delta::Remove( move.movedKey(),     move.start()           ), 
    Delta::Place ( move.movedKey(),     move.end()             ),
    Delta::Remove( move.enpassantKey(), move.enpassantSquare() ),
  };
}

template<>
inline std::vector<Delta> pieceSquareDeltas<MoveType::Promotion>(const Move<MoveType::Promotion> move) {
  std::vector<Delta> deltas = {
    Delta::Remove( move.movedKey(),     move.start() ),
    Delta::Place ( move.promotionKey(), move.end()   ),
  };
  if (auto maybeCapturedKey = move.capturedKey(); maybeCapturedKey) {
    deltas.push_back(Delta::Remove( *maybeCapturedKey, move.end() ));
  }
  return deltas;
}

template<>
inline std::vector<Delta> pieceSquareDeltas<MoveType::Castle>(const Move<MoveType::Castle> move) {
  return {
    Delta::Remove( move.movedKey(),       move.start()          ),
    Delta::Place ( move.movedKey(),       move.end()            ),
    Delta::Remove( move.castledRookKey(), move.castledRookStart() ),
    Delta::Place ( move.castledRookKey(), move.castledRookEnd()   ),
  };
}

template<>
inline std::vector<Delta> pieceSquareDeltas<MoveType::DoublePawnPush>(const Move<MoveType::DoublePawnPush> move) {
  return {
    Delta::Remove( move.movedKey(), move.start() ),
    Delta::Place ( move.movedKey(), move.end()   ),
  };
}
