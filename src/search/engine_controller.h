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
    EngineController() noexcept : position_{}, hashEntries_{defaultHashEntries()} {}

    explicit EngineController(const Position& position) noexcept
        : position_{position}, hashEntries_{defaultHashEntries()}
    {
    }

    explicit EngineController(const std::string& fen) noexcept
        : position_{fen}, hashEntries_{defaultHashEntries()}
    {
    }

    // Precondition for search/playEngineMove: position_ has at least one legal move.
    SearchResult search(const SearchConfig& config, const std::atomic<bool>* stopFlag = nullptr,
                        Searcher::DepthInfoCallback onDepthCompleted = nullptr)
    {
        return Searcher::search(position_, config, ttFor(config), threadPoolFor(config), stopFlag,
                                std::move(onDepthCompleted));
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

    void setHashSizeMB(int hashMB) noexcept { hashEntries_ = hashEntriesFromMB(hashMB); }

    void clearTranspositionTable() noexcept
    {
        if (tt_)
            tt_->clear();
    }

    [[nodiscard]] const Position& position() const noexcept { return position_; }
    [[nodiscard]] Color turn() const noexcept { return position_.sideToMove(); }

  private:
    Position position_;
    std::unique_ptr<TranspositionTable> tt_;
    std::unique_ptr<ThreadPool> threadPool_;
    size_t hashEntries_;
    size_t ttSize_{0};

    [[nodiscard]] static size_t defaultHashEntries() noexcept
    {
        return (1UL << 20) + 7;
    }

    [[nodiscard]] static size_t hashEntriesFromMB(int hashMB) noexcept
    {
        constexpr size_t kEntryBytes = 12;
        const size_t bytes = static_cast<size_t>(std::max(hashMB, 1)) * 1024UL * 1024UL;
        return std::max(bytes / kEntryBytes, size_t{1024});
    }

    // The default table is ~12.6 MB and its occupied-bit pattern has to be
    // written on construction, so controllers that never search with a table
    // (perft, tests, LOW strength) should not pay for one. Released again on a
    // no-TT search rather than held idle; entries are key-verified, so losing
    // them only costs move-ordering quality on the next search.
    TranspositionTable* ttFor(const SearchConfig& config)
    {
        if (config.options.useTT)
        {
            if (!tt_ || ttSize_ != hashEntries_)
            {
                tt_ = std::make_unique<TranspositionTable>(hashEntries_);
                ttSize_ = hashEntries_;
            }
        }
        else if (tt_)
        {
            tt_.reset();
            ttSize_ = 0;
        }
        return tt_.get();
    }

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
        std::array<int, static_cast<int>(StrengthLevel::NUM_LEVELS)> timeBudgetsMS = {100, 1500,
                                                                                      8000};
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
