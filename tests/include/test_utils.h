#pragma once

#include <gtest/gtest.h>

#include "move_factory.h"
#include "position.h"

class ChessTestFixture : public ::testing::Test
{
  protected:
    using Normal = Move<MoveType::Normal>;
    using DoublePush = Move<MoveType::DoublePawnPush>;
    using Enpassant = Move<MoveType::Enpassant>;
    using Promotion = Move<MoveType::Promotion>;
    using Castle = Move<MoveType::Castle>;

    Position pos;

    void loadStartingPosition() { pos = Position::fromStartingPosition(); }
    void loadFen(const std::string& fen) { pos = Position(fen); }

    [[nodiscard]] Normal normal(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Normal>(pos, from, to);
    }

    [[nodiscard]] Promotion promotion(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Promotion>(pos, from, to);
    }

    [[nodiscard]] Enpassant enpassant(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Enpassant>(pos, from, to);
    }

    [[nodiscard]] DoublePush doublePush(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::DoublePawnPush>(pos, from, to);
    }

    [[nodiscard]] Castle castle(Square from, Square to) const
    {
        return MoveFactory::createMove<MoveType::Castle>(pos, from, to);
    }
};
