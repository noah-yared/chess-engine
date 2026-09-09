#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>

#include "board/position.h"
#include "concurrency/thread_pool.h"
#include "move/move.h"
#include "search/search_types.h"
#include "search/searcher.h"
#include "search/transposition_table.h"

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

    SearchResult search(const SearchConfig& config, const std::atomic<bool>* stopFlag = nullptr,
                        Searcher::DepthInfoCallback onDepthCompleted = nullptr)
    {
        return Searcher::search(position_, config, ttFor(config), threadPoolFor(config), stopFlag,
                                std::move(onDepthCompleted));
    }

    SearchResult search(int depth = DEFAULT_SEARCH_DEPTH)
    {
        return search(SearchConfig::fixedDepth(depth));
    }

    Move playEngineMove(const SearchConfig& config)
    {
        auto result = search(config);
        advance(result.bestMove);
        return result.bestMove;
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
};
