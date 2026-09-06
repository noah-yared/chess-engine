#pragma once

#include <array>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <type_traits>

#include "board/position.h"
#include "concurrency/thread_pool.h"
#include "move/move.h"
#include "search/search_types.h"
#include "search/searcher.h"
#include "search/transposition_table.h"

enum class StrengthLevel
{
    LOW,
    MEDIUM,
    HIGH,
    NUM_LEVELS
};

class EngineController
{
  public:
    EngineController() noexcept : position_{}, tt_{} {};
    explicit EngineController(const Position& position) : position_{position}, tt_{} {};
    explicit EngineController(const std::string& fen) : position_{fen}, tt_{} {};

    // Precondition for search/playEngineMove: position_ has at least one legal move.
    SearchResult search(const SearchConfig& config)
    {
        return Searcher::search(position_, config, config.options.useTT ? &tt_ : nullptr,
                                threadPoolFor(config));
    }

    // Useful for quick tests.
    SearchResult search(int depth = DEFAULT_SEARCH_DEPTH)
    {
        return search(SearchConfig::fixedDepth(depth));
    }

    // Same precondition as search().
    Move playEngineMove(const SearchConfig& config)
    {
        auto result = search(config);
        advance(result.bestMove);
        return result.bestMove;
    }

    Move playEngineMove(StrengthLevel strength, int parallelism = 1)
    {
        return playEngineMove(buildStrengthConfig(strength).setParallelism(parallelism));
    }

    void advance(Move move) noexcept { position_.applyMove(move); }

    void setPosition(const std::string& fen) noexcept { position_ = Position(fen); }
    void setPosition(const Position& position) noexcept { position_ = position; }

    [[nodiscard]] const Position& position() const noexcept { return position_; }
    [[nodiscard]] Color turn() const noexcept { return position_.sideToMove(); }

  private:
    Position position_;
    TranspositionTable tt_;
    std::unique_ptr<ThreadPool> threadPool_;

    // Sequential search (parallelism == 1) does not attach a pool, so 1-thread
    // benches match the pre-YBWC path. Rebuild the pool if worker count changes.
    ThreadPool* threadPoolFor(const SearchConfig& config)
    {
        const int workers = clampSearchParallelism(config.limits.parallelism);
        if (workers <= 1)
        {
            threadPool_.reset();
            return nullptr;
        }
        if (!threadPool_ || threadPool_->numWorkers() != workers)
            threadPool_ = std::make_unique<ThreadPool>(workers);
        return threadPool_.get();
    }

    [[nodiscard]] static int computeTimeBudgetMS(StrengthLevel strength)
    {
        std::array<int, static_cast<int>(StrengthLevel::NUM_LEVELS)> timeBudgetsMS = {100, 1500, 8000};
        return timeBudgetsMS[static_cast<int>(strength)];
    }

    [[nodiscard]] static SearchConfig buildStrengthConfig(StrengthLevel strength)
    {
        switch (strength)
        {
        case StrengthLevel::LOW:
            return SearchConfig::fixedTime(computeTimeBudgetMS(strength))
                .withoutQuiescence() // misses tactics/exchanges
                .withoutTT();        // slow down search
        case StrengthLevel::MEDIUM:
        case StrengthLevel::HIGH:
            return SearchConfig::fixedTime(computeTimeBudgetMS(strength));
        default:
            // should not reach this case
            assert(false && "Invalid strength passed in!");
            std::abort();
        }
    }
};
