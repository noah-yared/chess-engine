#pragma once

#include "board/board_state.h"
#include "board/squares.h"
#include "move/move.h"

template <MoveType mType>
inline void updateEnpassantSquare(const Move move, BoardState& state) noexcept
{
    if constexpr (mType == MoveType::DoublePawnPush)
        state.setEnpassantSquare(move.enpassantTargetSquare());
    else
        state.setEnpassantSquare(std::nullopt);
}

template <MoveType mType>
inline void updateCastlingPrivs(const Move move, BoardState& state) noexcept
{
    if constexpr (mType == MoveType::Castle)
    {
        move.side() == Color::WHITE ? state.stripCastlingPrivileges<'K', 'Q'>()
                                    : state.stripCastlingPrivileges<'k', 'q'>();
    }
    else
    {
        if (move.moved() == PieceType::KING)
        {
            move.side() == Color::WHITE ? state.stripCastlingPrivileges<'K', 'Q'>()
                                        : state.stripCastlingPrivileges<'k', 'q'>();
        }
        else if (move.moved() == PieceType::ROOK)
        {
            if (isSquareOnLeftEdge(move.start()))
                move.side() == Color::WHITE ? state.stripCastlingPrivileges<'Q'>()
                                            : state.stripCastlingPrivileges<'q'>();
            else if (isSquareOnRightEdge(move.start()))
                move.side() == Color::WHITE ? state.stripCastlingPrivileges<'K'>()
                                            : state.stripCastlingPrivileges<'k'>();
        }

        switch (move.end())
        {
        case Square::A1:
            state.stripCastlingPrivileges<'Q'>();
            break;
        case Square::H1:
            state.stripCastlingPrivileges<'K'>();
            break;
        case Square::A8:
            state.stripCastlingPrivileges<'q'>();
            break;
        case Square::H8:
            state.stripCastlingPrivileges<'k'>();
            break;
        }
    }
}

template <MoveType mType>
inline void updateTurn(const Move move, BoardState& state) noexcept
{
    state.updateTurn();
}
