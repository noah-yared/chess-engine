#pragma once

#include "board/constants.h"
#include "move/move.h"

struct SearchConfig
{
  private:
    struct SearchLimits
    {
        int maxDepth = DEFAULT_SEARCH_DEPTH;
        int timeLimitMS = DEFAULT_SEARCH_TIME_BUDGET;
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
        config.limits.timeLimitMS = timeLimitMS;
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
};

struct SearchStats
{
    u64 nodesSearched = 0ULL;
    u64 ttHits = 0ULL;
    u64 cutoffs = 0ULL;
};

struct SearchResult
{
    MoveVariant bestMove;
    int score = 0;
    bool aborted = false;
    SearchStats stats;
};
