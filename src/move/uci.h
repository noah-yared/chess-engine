#pragma once

#include <cstdlib>
#include <string>

#include "board/constants.h"
#include "board/pieces.h"
#include "board/position.h"
#include "board/squares.h"
#include "move/move.h"
#include "move/move_utils.h"

// Uses the current state of pos to infer move type and captured piece.
[[nodiscard]] inline MoveVariant uciToMove(const std::string& uci, const Position& pos) noexcept
{
    constexpr auto normal = MoveType::Normal;
    constexpr auto promotion = MoveType::Promotion;
    constexpr auto doublePush = MoveType::DoublePawnPush;
    constexpr auto enpassant = MoveType::Enpassant;
    constexpr auto castle = MoveType::Castle;

    int from = algebraicNotationToIndex(uci.substr(0, 2)),
        to = algebraicNotationToIndex(uci.substr(2, 2));
    auto movedPiece = pos.getPieceOccupyingSquare(from);
    if (uci.size() == 5)
        return MoveFactory::createMove<promotion>(pos, from, to);
    if (auto maybeEnpassantSq = pos.maybeEnpassantSquare();
        maybeEnpassantSq && (to == *maybeEnpassantSq) && (movedPiece == PieceType::PAWN))
        return MoveFactory::createMove<enpassant>(pos, from, to);
    if (std::abs(from - to) == FILES * 2 && movedPiece == PieceType::PAWN)
        return MoveFactory::createMove<doublePush>(pos, from, to);
    if (std::abs(from - to) == 2 && movedPiece == PieceType::KING)
        return MoveFactory::createMove<castle>(pos, from, to);
    return MoveFactory::createMove<normal>(pos, from, to);
}
