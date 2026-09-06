#include <gtest/gtest.h>

#include "board/position.h"
#include "move/move_generator.h"
#include "move/move_list.h"
#include "test_utils.h"

// Node counts are the published values from the Chess Programming Wiki.
class PerftTest : public ChessTestFixture
{
  protected:
    u64 perft(int depth)
    {
        if (depth == 1)
            return legalMoves().size();

        u64 total = 0;
        auto posCopy = pos; // backup position
        for (const Move move : legalMoves())
        {
            // save snapshot before applying move
            auto snapshot = pos.getStateSnapshot();

            pos.applyMove(move);
            total += perft(depth - 1);
            pos.undoMove(move, snapshot);

            // verify position is restored
            EXPECT_EQ(posCopy, pos)
                << "Position did not return to original state after applying & undoing move "
                << "\"" << move.uci() << "\""
                << " on \"" << posCopy.toFen() << "\"\n";
        }
        return total;
    }
};

TEST_F(PerftTest, StartingPosition_AllLegalMoves)
{
    loadStartingPosition();
    EXPECT_EQ(perft(1), 20);
    EXPECT_EQ(perft(2), 400);
    EXPECT_EQ(perft(3), 8902);
    EXPECT_EQ(perft(4), 197281);
    EXPECT_EQ(perft(5), 4865609);
    // EXPECT_EQ(perft(6), 119060324);
}

TEST_F(PerftTest, Position2_AllLegalMoves)
{
    loadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    EXPECT_EQ(perft(1), 48);
    EXPECT_EQ(perft(2), 2039);
    EXPECT_EQ(perft(3), 97862);
    EXPECT_EQ(perft(4), 4085603);
    // EXPECT_EQ(perft(5), 193690690);
}

TEST_F(PerftTest, Position3_AllLegalMoves)
{
    loadFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    EXPECT_EQ(perft(1), 14);
    EXPECT_EQ(perft(2), 191);
    EXPECT_EQ(perft(3), 2812);
    EXPECT_EQ(perft(4), 43238);
    EXPECT_EQ(perft(5), 674624);
    EXPECT_EQ(perft(6), 11030083);
    // EXPECT_EQ(perft(7), 178633661);
}

TEST_F(PerftTest, Position4_AllLegalMoves)
{
    loadFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    EXPECT_EQ(perft(1), 6);
    EXPECT_EQ(perft(2), 264);
    EXPECT_EQ(perft(3), 9467);
    EXPECT_EQ(perft(4), 422333);
    EXPECT_EQ(perft(5), 15833292);
    // EXPECT_EQ(perft(6), 706045033);
}

TEST_F(PerftTest, Position5_AllLegalMoves)
{
    loadFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    EXPECT_EQ(perft(1), 44);
    EXPECT_EQ(perft(2), 1486);
    EXPECT_EQ(perft(3), 62379);
    EXPECT_EQ(perft(4), 2103487);
    EXPECT_EQ(perft(5), 89941194);
}

TEST_F(PerftTest, Position6_AllLegalMoves)
{
    loadFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    EXPECT_EQ(perft(1), 46);
    EXPECT_EQ(perft(2), 2079);
    EXPECT_EQ(perft(3), 89890);
    EXPECT_EQ(perft(4), 3894594);
    // EXPECT_EQ(perft(5), 164075551);
}
