#include <gtest/gtest.h>

#include <chrono>
#include <string>

#include "board/constants.h"
#include "move/move_generator.h"
#include "search/engine_controller.h"
#include "search/searcher.h"
#include "search/strength.h"
#include "search/transposition_table.h"
#include "test_utils.h"

class SearchTest : public ChessTestFixture
{
  protected:
    [[nodiscard]] bool isLegalMove(const MoveVariant& move) const
    {
        return legalMoves().contains(move);
    }
};

TEST_F(SearchTest, SearcherReturnsLegalMoveForStartingPosition)
{
    loadStartingPosition();
    TranspositionTable tt(257);
    auto config = SearchConfig::fixedDepth(1);

    auto result = Searcher::search(pos, config, &tt);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
}

TEST_F(SearchTest, SearcherCanSearchWithoutTranspositionTable)
{
    loadStartingPosition();
    const auto original = pos;
    auto config = SearchConfig::fixedDepth(2);

    auto result = Searcher::search(pos, config, nullptr);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
    EXPECT_EQ(pos, original);
}

TEST_F(SearchTest, SearcherCanSearchWithTimeManagement)
{
    loadStartingPosition();
    TranspositionTable tt(257);
    auto config = SearchConfig::fixedTime(50, 3);

    auto result = Searcher::search(pos, config, &tt);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
}

TEST_F(SearchTest, TimedSearchWithHighDepthStopsOnTime)
{
    loadStartingPosition();
    TranspositionTable tt(257);
    auto config = SearchConfig::fixedTime(20, 64 /* very high max depth */);

    const auto start = std::chrono::steady_clock::now();
    auto result = Searcher::search(pos, config, &tt);
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
    EXPECT_LT(elapsed.count(), 250); // make it loose to account for variability
                                     // from polling periods, os scheduler/hardware, etc.
}

TEST_F(SearchTest, SearcherCanSearchWithoutQuiescence)
{
    loadStartingPosition();
    TranspositionTable tt(257);
    auto config = SearchConfig::fixedDepth(2).withoutQuiescence();

    auto result = Searcher::search(pos, config, &tt);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
}

TEST_F(SearchTest, SearcherDoesNotMutateRootPosition)
{
    loadStartingPosition();
    const auto original = pos;
    TranspositionTable tt(257);
    auto config = SearchConfig::fixedDepth(1);

    (void)Searcher::search(pos, config, &tt);

    EXPECT_EQ(pos, original);
}

TEST_F(SearchTest, SearcherReturnsLegalMoveForBlackToMove)
{
    loadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    TranspositionTable tt(257);
    auto config = SearchConfig::fixedDepth(1);

    auto result = Searcher::search(pos, config, &tt);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
}

TEST_F(SearchTest, EngineControllerAppliesPlayedMove)
{
    EngineController controller{std::string(STARTING_FEN)};
    Position expected = Position::fromStartingPosition();

    auto move = controller.playEngineMove(SearchConfig::fixedDepth(1));
    std::visit([&expected](auto&& m) noexcept { expected.applyMove(m); }, move);

    EXPECT_EQ(controller.position(), expected);
}
