#pragma once

#include "board/constants.h"
#include "move/move.h"

struct SearchConfig
{
  private:
    struct SearchLimits
    {
        int maxDepth = DEFAULT_SEARCH_DEPTH;
        int timeLimitMS = -1;
    };

    struct SearchOptions
    {
        bool use_quiescence = true;
        bool use_tt = true;
        // TODO: add support
        // bool use_time_mangement = true;
    };

  public:
    SearchLimits limits;
    SearchOptions options;

    // simple constructor for disabled time control, enabled quiescence/tt
    SearchConfig(int maxDepth) : limits{.maxDepth = maxDepth}, options{} {};
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
