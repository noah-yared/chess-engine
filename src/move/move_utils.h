#pragma once

#include <optional>

#include "board/pieces.h"
#include "board/position.h"
#include "move/move.h"

class MoveFactory
{
  public:
    // for uci parsing
    template <MoveType mType>
    static Move createMove(const Position& pos, int start, int end,
                           PieceType promotionPiece = PieceType::QUEEN) noexcept
    {
        if constexpr (!CanCapture<mType>)
            return Move(mType, start, end, pos.sideToMove(), pos.getPieceOccupyingSquare(start),
                        std::nullopt, promotionPiece);
        else
            return Move(mType, start, end, pos.sideToMove(), pos.getPieceOccupyingSquare(start),
                        pos.getPieceOccupyingSquare(end, opposite(pos.sideToMove())),
                        promotionPiece);
    }

    // for move generation (where we know the piece type at compile time)
    template <MoveType mType, PieceType pType, Color color>
    static Move createMove(const Position& pos, int start, int end,
                           PieceType promotionPiece = PieceType::QUEEN) noexcept
    {
        if constexpr (!CanCapture<mType>)
            return Move(mType, start, end, color, pType, std::nullopt, promotionPiece);
        else
            return Move(mType, start, end, color, pType,
                        pos.getPieceOccupyingSquare(end, opposite<color>()), promotionPiece);
    }
};
