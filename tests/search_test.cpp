#include <gtest/gtest.h>

#include <string>

#include "board/constants.h"
#include "move/move_generator.h"
#include "search/difficulty.h"
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
    SearchConfig config(1);

    auto result = Searcher::search(pos, config, &tt);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
}

TEST_F(SearchTest, SearcherCanSearchWithoutTranspositionTable)
{
    loadStartingPosition();
    const auto original = pos;
    SearchConfig config(2);

    auto result = Searcher::search(pos, config, nullptr);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
    EXPECT_EQ(pos, original);
}

TEST_F(SearchTest, SearcherDoesNotMutateRootPosition)
{
    loadStartingPosition();
    const auto original = pos;
    TranspositionTable tt(257);
    SearchConfig config(1);

    (void)Searcher::search(pos, config, &tt);

    EXPECT_EQ(pos, original);
}

TEST_F(SearchTest, SearcherReturnsLegalMoveForBlackToMove)
{
    loadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    TranspositionTable tt(257);
    SearchConfig config(1);

    auto result = Searcher::search(pos, config, &tt);

    EXPECT_TRUE(isLegalMove(result.bestMove));
    EXPECT_GT(result.stats.nodesSearched, 0ULL);
}

TEST_F(SearchTest, EngineControllerAppliesPlayedMove)
{
    EngineController controller{std::string(STARTING_FEN)};
    Position expected = Position::fromStartingPosition();

    auto move = controller.playEngineMove(1);
    std::visit([&expected](auto&& m) noexcept { expected.applyMove(m); }, move);

    EXPECT_EQ(controller.position(), expected);
}
