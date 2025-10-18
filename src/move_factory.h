#pragma once

#include "move.h"
#include "pieces.h"
#include "position.h"

class MoveFactory
{
  public:
    // for uci parsing
    template <MoveType mType>
    static Move<mType> createMove(const Position& pos, int start, int end) noexcept
    {
        if constexpr (!CanCapture<mType>)
            return Move<mType>(start, end, pos.sideToMove(), pos.getPieceOccupyingSquare(start));
        else
        {
            auto capturedPiece = pos.getPieceOccupyingSquare(end, opposite(pos.sideToMove()));
            return capturedPiece != PieceType::NONE
                       ? Move<mType>(start, end, pos.sideToMove(),
                                     pos.getPieceOccupyingSquare(start), capturedPiece)
                       : Move<mType>(start, end, pos.sideToMove(),
                                     pos.getPieceOccupyingSquare(start));
        }
    }

    // for move generation (where we know the piece type at compile time)
    template <MoveType mType, PieceType pType, Color color>
    static Move<mType> createMove(const Position& pos, int start, int end) noexcept
    {
        if constexpr (!CanCapture<mType>)
            return Move<mType>(start, end, color, pType);
        else
        {
            auto capturedPiece = pos.getPieceOccupyingSquare(end, opposite<color>());
            return capturedPiece != PieceType::NONE
                       ? Move<mType>(start, end, color, pType, capturedPiece)
                       : Move<mType>(start, end, color, pType);
        }
    }
};
