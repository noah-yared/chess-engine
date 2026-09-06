#pragma once

#include <algorithm>
#include <thread>

#include "board/constants.h"
#include "move/move.h"

// hardware_concurrency() may return 0 when the OS does not report a count.
[[nodiscard]] inline int maxSearchParallelism() noexcept
{
    const unsigned hw = std::thread::hardware_concurrency();
    return hw > 0 ? static_cast<int>(hw) : MIN_SEARCH_PARALLELISM;
}

[[nodiscard]] inline int clampSearchParallelism(int workers) noexcept
{
    return std::clamp(workers, MIN_SEARCH_PARALLELISM, maxSearchParallelism());
}

struct SearchConfig
{
  private:
    struct SearchLimits
    {
        int maxDepth = DEFAULT_SEARCH_DEPTH;
        int timeLimitMS = DEFAULT_SEARCH_TIME_BUDGET;
        int parallelism = 1;
    };

    struct SearchOptions
    {
        bool useQuiescence = true;
        bool useTT = true;
        bool useTimeManagement = false;
    };

  public:
    SearchLimits limits;
    SearchOptions options;

    static SearchConfig fixedDepth(int depth = DEFAULT_SEARCH_DEPTH)
    {
        SearchConfig config;
        config.limits.maxDepth = depth;
        config.options.useTimeManagement = false;
        return config;
    }

    static SearchConfig fixedTime(int timeLimitMS = DEFAULT_SEARCH_TIME_BUDGET,
                                  int maxDepth = MAX_SEARCH_DEPTH)
    {
        SearchConfig config;
        config.limits.maxDepth = maxDepth;
        // Guard against caller-provided budgets that are too small for a useful timed search.
        config.limits.timeLimitMS = std::max(timeLimitMS, MIN_SEARCH_TIME_BUDGET);
        config.options.useTimeManagement = true;
        return config;
    }

    [[nodiscard]] SearchConfig& withoutTT()
    {
        options.useTT = false;
        return *this;
    }

    [[nodiscard]] SearchConfig& withoutQuiescence()
    {
        options.useQuiescence = false;
        return *this;
    }

    [[nodiscard]] SearchConfig& setParallelism(int workers)
    {
        limits.parallelism = clampSearchParallelism(workers);
        return *this;
    }
};

struct SearchStats
{
    u64 nodesSearched = 0ULL;
    u64 ttHits = 0ULL;
    u64 cutoffs = 0ULL;

    SearchStats& operator+=(const SearchStats& other) noexcept
    {
        nodesSearched += other.nodesSearched;
        ttHits += other.ttHits;
        cutoffs += other.cutoffs;
        return *this;
    }
};

struct SearchResult
{
    Move bestMove;
    int score = 0;
    bool aborted = false;
    SearchStats stats;
};
