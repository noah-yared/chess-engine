#pragma once

#include "board_utils.h"
#include "move.h"
#include "move_factory.h"
#include "position.h"

// uses current state of instance position object pos
[[nodiscard]] inline MoveVariant uciToMove(const std::string& uci, const Position& pos) noexcept
{
    constexpr auto normal = MoveType::Normal;
    constexpr auto promotion = MoveType::Promotion;
    constexpr auto doublePush = MoveType::DoublePawnPush;
    constexpr auto enpassant = MoveType::Enpassant;
    constexpr auto castle = MoveType::Castle;

    int from = algebraicNotationToIndex(uci.substr(0, 2)),
        to = algebraicNotationToIndex(uci.substr(2, 2));
    auto movedPiece = pos.getPieceOccupyingSquare(from),
         capturedPiece = pos.getPieceOccupyingSquare(to);
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
