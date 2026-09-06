#pragma once

#include <gtest/gtest.h>

#include "board/position.h"
#include "move/move_generator.h"
#include "move/move_list.h"
#include "move/move_utils.h"

class ChessTestFixture : public ::testing::Test
{
  private:
    mutable MoveList moveBuffer_;

  protected:
    Position pos;

    void loadStartingPosition() { pos = Position::fromStartingPosition(); }
    void loadFen(const std::string& fen) { pos = Position(fen); }

    [[nodiscard]] MoveList legalMoves() const
    {
        moveBuffer_.clear();
        pos.isWhiteToMove() ? MoveGenerator::pushLegalMoves<Color::WHITE>(pos, moveBuffer_)
                            : MoveGenerator::pushLegalMoves<Color::BLACK>(pos, moveBuffer_);
        return moveBuffer_;
    }

    [[nodiscard]] Move normal(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Normal>(pos, from, to);
    }

    [[nodiscard]] Move promotion(Square from, Square to,
                                 PieceType promotionPiece = PieceType::QUEEN) const
    {
        return MoveFactory::createMove<MoveType::Promotion>(pos, from, to, promotionPiece);
    }

    [[nodiscard]] Move enpassant(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Enpassant>(pos, from, to);
    }

    [[nodiscard]] Move doublePush(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::DoublePawnPush>(pos, from, to);
    }

    [[nodiscard]] Move castle(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Castle>(pos, from, to);
    }
};
