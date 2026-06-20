#include <gtest/gtest.h>

#include <variant>

#include "move/move_generator.h"
#include "move/move_list.h"
#include "board/position.h"
#include "test_utils.h"

class PerftTest : public ChessTestFixture
{
  protected:
    u64 perft(int depth)
    {
        if (depth == 1)
            return legalMoves().size();

        u64 total = 0;
        auto posCopy = pos; // backup position
        for (const auto& move : legalMoves())
        {
            // save snapshot before applying move
            auto snapshot = pos.getStateSnapshot();

            std::visit([&](auto&& m) { pos.applyMove(m); }, move);
            total += perft(depth - 1);
            std::visit([&](auto&& m) { pos.undoMove(m, snapshot); }, move);
            
            // verify position is restored
            EXPECT_EQ(posCopy, pos) << "Position did not return to original state after applying & undoing move "
                                    << "\"" << std::visit([&](auto&& m) { return m.uci(); }, move) << "\""
                                    << " on \"" << posCopy.toFen() << "\"\n";
        }
        return total;
    }
};

TEST_F(PerftTest, StartingPosition_AllLegalMovesExcludingNonQueenPromotions)
{
    loadStartingPosition();
    EXPECT_EQ(perft(1), 20);
    EXPECT_EQ(perft(2), 400);
    EXPECT_EQ(perft(3), 8902);
    EXPECT_EQ(perft(4), 197281);
    EXPECT_EQ(perft(5), 4865609);
    EXPECT_EQ(perft(6), 119060324);
}

TEST_F(PerftTest, Position2_AllLegalMovesExcludingNonQueenPromotions)
{
    loadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(perft(1), 48);
    EXPECT_EQ(perft(2), 2039);
    EXPECT_EQ(perft(3), 97862);
    EXPECT_EQ(perft(4), 4074224);
    EXPECT_EQ(perft(5), 193301718);
}

TEST_F(PerftTest, Position3_AllLegalMovesExcludingNonQueenPromotions)
{
    loadFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(perft(1), 14);
    EXPECT_EQ(perft(2), 191);
    EXPECT_EQ(perft(3), 2812);
    EXPECT_EQ(perft(4), 43238);
    EXPECT_EQ(perft(5), 674624);
    EXPECT_EQ(perft(6), 11024419);
    EXPECT_EQ(perft(7), 178447267);
}

TEST_F(PerftTest, Position4_AllLegalMovesExcludingNonQueenPromotions)
{
    loadFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(1), 6);
    EXPECT_EQ(perft(2), 228);
    EXPECT_EQ(perft(3), 8087);
    EXPECT_EQ(perft(4), 320802);
    EXPECT_EQ(perft(5), 11875685);
    EXPECT_EQ(perft(6), 481688280);
}

TEST_F(PerftTest, Position5_AllLegalMovesExcludingNonQueenPromotions)
{
    loadFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(1), 41);
    EXPECT_EQ(perft(2), 1373);
    EXPECT_EQ(perft(3), 54007);
    EXPECT_EQ(perft(4), 1806790);
    EXPECT_EQ(perft(5), 72590339);
}

TEST_F(PerftTest, Position6_AllLegalMovesExcludingNonQueenPromotions)
{
    loadFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft(1), 46);
    EXPECT_EQ(perft(2), 2079);
    EXPECT_EQ(perft(3), 89890);
    EXPECT_EQ(perft(4), 3894594);
    EXPECT_EQ(perft(5), 164075551);
}

