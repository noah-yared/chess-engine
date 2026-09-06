#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <string>

#include "board/constants.h"
#include "move/move_generator.h"
#include "search/engine_controller.h"
#include "search/searcher.h"
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
    auto config = SearchConfig::fixedTime(20, MAX_SEARCH_DEPTH /* very high max depth */);

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

TEST_F(SearchTest, MultiWorkerFixedDepthMatchesSingleWorkerScore)
{
    // Depth 4 is MIN_SPLIT_DEPTH, so root (and some interior nodes at 5) split.
    constexpr int kWorkers = 4;
    constexpr int kDepth = 5;
    const std::array fens = {
        std::string(STARTING_FEN),
        std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1"),
        std::string("r6R/2pbpBk1/1P1B1N2/6q1/4Q3/2nn1p2/1PK1NbP1/R6r w - - 0 1"),
    };
    const auto config = SearchConfig::fixedDepth(kDepth).withoutTT();
    const auto parallelConfig = SearchConfig::fixedDepth(kDepth).withoutTT().setParallelism(kWorkers);

    for (const auto& fen : fens)
    {
        loadFen(fen);
        const auto original = pos;
        EngineController sequential(pos);
        EngineController parallel(pos);

        const auto sequentialResult = sequential.search(config);
        const auto parallelResult = parallel.search(parallelConfig);

        EXPECT_EQ(sequentialResult.score, parallelResult.score) << fen;
        EXPECT_TRUE(isLegalMove(sequentialResult.bestMove)) << fen;
        EXPECT_TRUE(isLegalMove(parallelResult.bestMove)) << fen;
        EXPECT_GT(parallelResult.stats.nodesSearched, 0ULL) << fen;
        EXPECT_EQ(pos, original) << fen;
        EXPECT_EQ(sequential.position(), original) << fen;
        EXPECT_EQ(parallel.position(), original) << fen;
    }
}

TEST_F(SearchTest, MultiWorkerTimedSearchWithHighDepthStopsOnTime)
{
    loadStartingPosition();
    EngineController engine(pos);
    auto config = SearchConfig::fixedTime(20, MAX_SEARCH_DEPTH).setParallelism(4);

    const auto start = std::chrono::steady_clock::now();
    auto result = engine.search(config);
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
    EXPECT_LT(elapsed.count(), 250);
}
