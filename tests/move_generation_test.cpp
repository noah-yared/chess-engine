#include <gtest/gtest.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iterator>
#include <string>
#include <type_traits>
#include <unordered_set>

#include "board_utils.h"
#include "move_generator.h"
#include "move_utils.h"
#include "position.h"
#include "test_utils.h"

class MoveGenerationTest : public ChessTestFixture
{
  protected:
    template <typename... UciMoveTypes>
        requires(std::conjunction_v<std::is_convertible<
                     UciMoveTypes, std::string>...>) // make sure all types are strings
    void expectLegalMoves(const UciMoveTypes&... uciMoves)
    {
        MoveList expectedMoves(uciToMove(uciMoves, pos)...), actualMoves {};
        pos.isWhiteToMove() ? MoveGenerator::pushLegalMoves<Color::WHITE>(pos, actualMoves)
                            : MoveGenerator::pushLegalMoves<Color::BLACK>(pos, actualMoves);
        EXPECT_EQ(actualMoves, expectedMoves) << diff(actualMoves, expectedMoves);
    }

  private:
    [[nodiscard]] static std::string diff(const MoveList& actual, const MoveList& expected)
    {
        std::unordered_set<MoveVariant> actualSet(actual.begin(), actual.end()),
            expectedSet(expected.begin(), expected.end());

        // compute set differences
        std::unordered_set<MoveVariant> extraneous, missing;
        std::copy_if(actualSet.begin(), actualSet.end(),
                     std::inserter(extraneous, extraneous.begin()),
                     [&](const auto& item) { return !expectedSet.count(item); });
        std::copy_if(expectedSet.begin(), expectedSet.end(),
                     std::inserter(missing, missing.begin()),
                     [&](const auto& item) { return !actualSet.count(item); });

        // write diffs to stream, '+' indicates missing moves in actual from expected,
        // '-' indicates extraneous moves in actual but not in expected
        std::stringstream ss;
        ss << "diff:\n";
        for (const auto& move : extraneous)
            std::visit([&ss](auto&& arg) { ss << '-' << arg.uci() << '\n'; }, move);
        for (const auto& move : missing)
            std::visit([&ss](auto&& arg) { ss << '+' << arg.uci() << '\n'; }, move);
        return ss.str();
    }
};

TEST_F(MoveGenerationTest, HandlesStartingBoard)
{
    loadStartingPosition();
    expectLegalMoves("a2a3", "a2a4", "b1a3", "b1c3", "b2b3", "b2b4", "c2c3", "c2c4", "d2d3", "d2d4",
                     "e2e3", "e2e4", "f2f3", "f2f4", "g1f3", "g1h3", "g2g3", "g2g4", "h2h3",
                     "h2h4");
}

TEST_F(MoveGenerationTest, HandlesPromotion_WhitePawn)
{
    loadFen("3K4/PP1p1N1n/bp2pk1P/PN1p1p2/1p3PBb/2PPB2q/4n3/1r1Q2Rr w - - 0 4");
    expectLegalMoves("a5b6", "a7a8q", "b5a3", "b5c7", "b5d4", "b5d6", "b7b8q", "c3b4", "c3c4",
                     "d1a4", "d1b1", "d1b3", "d1c1", "d1c2", "d1d2", "d1e1", "d1e2", "d1f1", "d3d4",
                     "d8c7", "d8c8", "d8d7", "d8e8", "e3b6", "e3c1", "e3c5", "e3d2", "e3d4", "e3f2",
                     "f7d6", "f7e5", "f7g5", "f7h8", "g1e1", "g1f1", "g1g2", "g1g3", "g1h1", "g4e2",
                     "g4f3", "g4f5", "g4h3", "g4h5");
}

TEST_F(MoveGenerationTest, HandlesEnpassant_WhitePawnOnE5)
{
    loadFen("8/PPK2N1n/bp2k2P/PN1pPp2/1p4Bb/2PPB2q/4n3/1r1Q2Rr w - d6 0 8");
    expectLegalMoves("a5b6", "a7a8q", "b5a3", "b5d4", "b5d6", "b7b8q", "c3b4", "c3c4", "c7b6",
                     "c7b8", "c7c6", "c7c8", "d1a4", "d1b1", "d1b3", "d1c1", "d1c2", "d1d2", "d1e1",
                     "d1e2", "d1f1", "d3d4", "e3b6", "e3c1", "e3c5", "e3d2", "e3d4", "e3f2", "e3f4",
                     "e3g5", "e5d6", "f7d6", "f7d8", "f7g5", "f7h8", "g1e1", "g1f1", "g1g2", "g1g3",
                     "g1h1", "g4e2", "g4f3", "g4f5", "g4h3", "g4h5");
}

TEST_F(MoveGenerationTest, HandlesBlackKingInCheck_OnlyEscapeMoves)
{
    loadFen("1rbbN3/Kpp2p1Q/1n4nP/BP1BPp2/Pk2ppPp/3PN2p/2RR1PP1/1q5r b - - 0 1");
    expectLegalMoves("b4a3", "b4a4", "b4a5");
}

TEST_F(MoveGenerationTest, HandlesWhiteKingInCheck_AlliesMustCapture)
{
    loadFen("2qb3R/rn3kPB/pR3n1p/P2P2bK/2p1P3/1PBPp1pP/1NrppPp1/3N2Q1 w - - 0 1");
    expectLegalMoves("b6f6", "c3f6");
}

TEST_F(MoveGenerationTest, HandlesPositionWithManyPieceMoves)
{
    loadFen("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1");
    expectLegalMoves("a1a2", "a1a3", "a1a4", "a1a5", "a1a6", "a1a7", "a1a8", "a1b1", "a1c1", "a1d1",
                     "a1e1", "a1f1", "a1g1", "a1h1", "b2b3", "b2b4", "b2c3", "b6b7", "b6c7", "c2b3",
                     "c2c3", "c2d3", "d6a3", "d6b4", "d6c5", "d6c7", "d6e5", "d6e7", "d6f4", "d6g3",
                     "d6h2", "e2c1", "e2c3", "e2d4", "e2f4", "e2g1", "e2g3", "e4a4", "e4a8", "e4b4",
                     "e4b7", "e4c4", "e4c6", "e4d3", "e4d4", "e4d5", "e4e3", "e4e5", "e4e6", "e4e7",
                     "e4f3", "e4f4", "e4f5", "e4g4", "e4g6", "e4h4", "e4h7", "f6d5", "f6d7", "f6e8",
                     "f6g4", "f6g8", "f6h5", "f6h7", "f7a2", "f7b3", "f7c4", "f7d5", "f7e6", "f7e8",
                     "f7g6", "f7g8", "f7h5", "g2f3", "g2g3", "g2g4", "h8a8", "h8b8", "h8c8", "h8d8",
                     "h8e8", "h8f8", "h8g8", "h8h1", "h8h2", "h8h3", "h8h4", "h8h5", "h8h6",
                     "h8h7");
}

TEST_F(MoveGenerationTest, HandlesPositionWithMultipleQueens)
{
    loadFen("Q1BQ4/5Q2/7k/7p/6p1/2Q5/4R1PP/3R3K w - - 0 1");
    expectLegalMoves("a8a1", "a8a2", "a8a3", "a8a4", "a8a5", "a8a6", "a8a7", "a8b7", "a8b8", "a8c6",
                     "a8d5", "a8e4", "a8f3", "c3a1", "c3a3", "c3a5", "c3b2", "c3b3", "c3b4", "c3c1",
                     "c3c2", "c3c4", "c3c5", "c3c6", "c3c7", "c3d2", "c3d3", "c3d4", "c3e1", "c3e3",
                     "c3e5", "c3f3", "c3f6", "c3g3", "c3g7", "c3h3", "c3h8", "c8a6", "c8b7", "c8d7",
                     "c8e6", "c8f5", "c8g4", "d1a1", "d1b1", "d1c1", "d1d2", "d1d3", "d1d4", "d1d5",
                     "d1d6", "d1d7", "d1e1", "d1f1", "d1g1", "d8a5", "d8b6", "d8c7", "d8d2", "d8d3",
                     "d8d4", "d8d5", "d8d6", "d8d7", "d8e7", "d8e8", "d8f6", "d8f8", "d8g5", "d8g8",
                     "d8h4", "d8h8", "e2a2", "e2b2", "e2c2", "e2d2", "e2e1", "e2e3", "e2e4", "e2e5",
                     "e2e6", "e2e7", "e2e8", "e2f2", "f7a2", "f7a7", "f7b3", "f7b7", "f7c4", "f7c7",
                     "f7d5", "f7d7", "f7e6", "f7e7", "f7e8", "f7f1", "f7f2", "f7f3", "f7f4", "f7f5",
                     "f7f6", "f7f8", "f7g6", "f7g7", "f7g8", "f7h5", "f7h7", "g2g3", "h1g1", "h2h3",
                     "h2h4");
}

TEST_F(MoveGenerationTest, HandlesPositionWithMultiplePawnPromotions)
{
    loadFen("1n1r1b1r/P1P1P1P1/2BNq1k1/7R/3Q4/1P1N2K1/P1PBP3/5R2 w - - 15 45");
    expectLegalMoves(
        "a2a3", "a2a4", "a7a8q", "a7b8q", "b3b4", "c2c3", "c2c4", "c6a4", "c6a8", "c6b5", "c6b7",
        "c6d5", "c6d7", "c6e4", "c6e8", "c6f3", "c6g2", "c6h1", "c7b8q", "c7c8q", "c7d8q", "d2a5",
        "d2b4", "d2c1", "d2c3", "d2e1", "d2e3", "d2f4", "d2g5", "d2h6", "d3b2", "d3b4", "d3c1",
        "d3c5", "d3e1", "d3e5", "d3f2", "d3f4", "d4a1", "d4a4", "d4b2", "d4b4", "d4b6", "d4c3",
        "d4c4", "d4c5", "d4d5", "d4e3", "d4e4", "d4e5", "d4f2", "d4f4", "d4f6", "d4g1", "d4g4",
        "d4h4", "d6b5", "d6b7", "d6c4", "d6c8", "d6e4", "d6e8", "d6f5", "d6f7", "e2e3", "e2e4",
        "e7d8q", "e7e8q", "e7f8q", "f1a1", "f1b1", "f1c1", "f1d1", "f1e1", "f1f2", "f1f3", "f1f4",
        "f1f5", "f1f6", "f1f7", "f1f8", "f1g1", "f1h1", "g3f2", "g3f3", "g3f4", "g3g2", "g3h2",
        "g3h4", "g7f8q", "g7g8q", "g7h8q", "h5a5", "h5b5", "h5c5", "h5d5", "h5e5", "h5f5", "h5g5",
        "h5h1", "h5h2", "h5h3", "h5h4", "h5h6", "h5h7", "h5h8");
}

TEST_F(MoveGenerationTest, HandlesPinnedSlidingPiece_CanCapturePinner)
{
    loadFen("NR5r/3P1rpk/2n2P1p/pnP2q2/P1RPQP1N/BP1B1pKP/p1pp3p/b7 b - - 0 1");
    expectLegalMoves("a1b2", "a1c3", "a1d4", "b5a3", "b5a7", "b5c3", "b5c7", "b5d4", "b5d6",
                     "c2c1q", "c6a7", "c6b4", "c6b8", "c6d4", "c6d8", "c6e5", "c6e7", "d2d1q",
                     "f3f2", "f5e4", "f5g6", "f7d7", "f7e7", "f7f6", "f7f8", "g7f6", "g7g5", "g7g6",
                     "h2h1q", "h6h5", "h8b8", "h8c8", "h8d8", "h8e8", "h8f8", "h8g8");
}

TEST_F(MoveGenerationTest, HandlesAvailableCastle_WhiteLongSide)
{
    loadFen("r1b2rk1/p2p1ppp/4p3/2p1P3/2P2P2/2P5/P2NB1PP/R3K2R b Q - 1 17");
    expectLegalMoves("a7a5", "a7a6", "a8b8", "c8a6", "c8b7", "d7d5", "d7d6", "f7f5", "f7f6", "f8d8",
                     "f8e8", "g7g5", "g7g6", "g8h8", "h7h5", "h7h6");
}

TEST_F(MoveGenerationTest, HandlesAvailableCastle_BlackShortSide)
{
    loadFen("r1b1k2r/p2p1ppp/2p1p3/3nP3/1qP1NP2/3B4/PP1Q2PP/R3K2R b KQkq - 2 13");
    expectLegalMoves("a7a5", "a7a6", "a8b8", "b4a3", "b4a4", "b4a5", "b4b2", "b4b3", "b4b5", "b4b6",
                     "b4b7", "b4b8", "b4c3", "b4c4", "b4c5", "b4d2", "b4d6", "b4e7", "b4f8", "c6c5",
                     "c8a6", "c8b7", "d5b6", "d5c3", "d5c7", "d5e3", "d5e7", "d5f4", "d5f6", "d7d6",
                     "e8d8", "e8e7", "e8f8", "e8g8", "f7f5", "f7f6", "g7g5", "g7g6", "h7h5", "h7h6",
                     "h8f8", "h8g8");
}

TEST_F(MoveGenerationTest, HandlesWhiteKingCannotCastle_KnightAttackingCastlingLane)
{
    loadFen("r1b2rk1/p2p1ppp/2p1p3/4P3/2P2P2/2n5/PP1NB1PP/R3K1R1 w Q - 3 16");
    expectLegalMoves("a1b1", "a1c1", "a1d1", "a2a3", "a2a4", "b2b3", "b2b4", "b2c3", "c4c5", "d2b1",
                     "d2b3", "d2e4", "d2f1", "d2f3", "e1f1", "e1f2", "e2d1", "e2d3", "e2f1", "e2f3",
                     "e2g4", "e2h5", "f4f5", "g1f1", "g1h1", "g2g3", "g2g4", "h2h3", "h2h4");
}

TEST_F(MoveGenerationTest, HandlesCheckedWhiteKing_CannotCastle)
{
    loadFen("r1b1k2r/p2p1ppp/2p1p3/3nP3/2P1NP2/8/PP1qB1PP/R3K2R w KQkq - 0 14");
    expectLegalMoves("e1d2", "e1f1", "e1f2", "e4d2");
}

TEST_F(MoveGenerationTest, HandlesCheckmatedWhiteKing_TrappedOnRightEdge)
{
    loadFen("3b3R/rn3kPB/pR3B1p/P2P2bK/2p1P3/1P1Pp1pq/1NrppPp1/3N2Q1 w - - 0 2");
    expectLegalMoves();
}

TEST_F(MoveGenerationTest, HandlesCheckmatedBlackKing_TrappedInTopRightCorner)
{
    loadFen("NR5r/3P1r1k/2n2PBp/pnP5/P1RP1P1N/BP3pKP/p1pp3p/b7 b - - 0 3");
    expectLegalMoves();
}

TEST_F(MoveGenerationTest, HandlesStalemate_BlackKingTrappedInTopRightCornerByPawnAndKing)
{
    loadFen("7k/7P/6K1/5B2/8/8/8/8 b - - 100 226");
    expectLegalMoves();
}

TEST_F(MoveGenerationTest, HandlesStalemate_WhiteKingTrappedInBottomLeftCornerByPawnAndKing)
{
    loadFen("8/8/8/8/8/1k6/p7/K7 w - - 0 78");
    expectLegalMoves();
}

TEST_F(MoveGenerationTest, Regression_BulkTestingEdgeCase)
{
    // regression test that was failing due to a patched bug in move generation
    // (issue with correctly computing pawn diagonal attacks)
    loadFen("2kr3r/1b4p1/p1n1pq1p/1p1p4/P7/2P2N1P/1PB2PPB/R2QR1K1 b - - 0 21");
    expectLegalMoves("a6a5", "b5a4", "b5b4", "b7a8", "c6a5", "c6a7", "c6b4", "c6b8", "c6d4", "c6e5",
                     "c6e7", "c8d7", "d5d4", "d8d6", "d8d7", "d8e8", "d8f8", "d8g8", "e6e5", "f6c3",
                     "f6d4", "f6e5", "f6e7", "f6f3", "f6f4", "f6f5", "f6f7", "f6f8", "f6g5", "f6g6",
                     "f6h4", "g7g5", "g7g6", "h6h5", "h8e8", "h8f8", "h8g8", "h8h7");
}
