#pragma once

#include <atomic>
#include <cstddef>

#include "board/constants.h"
#include "board/pieces.h"
#include "board/position.h"
#include "search/move_ordering.h"

// Minimum remaining depth at a ROOT/MAIN node before younger brothers may be
// searched in parallel. Near-leaf splits spend more on Position copies and
// task enqueue than they recover.
constexpr int MIN_SPLIT_DEPTH = 4;

// Best score at a split, packed with the move index that produced it so a
// helper with a worse score cannot clobber a better best-move.
struct SplitBest
{
    int score = 0;
    int moveIndex = 0;
};

static_assert(std::atomic<SplitBest>::is_always_lock_free,
              "SplitBest CAS must be lock-free (no mutex under -fno-exceptions)");

// Shared state for a Young Brothers Wait split. Stack-allocated by the
// splitting thread; must outlive every submitted help task (the splitter
// waits on `pending` before returning).
//
// `position`, `moves`, `depthRemaining`, and `ply` are the node identity and
// are immutable after construction. Helpers copy `position` into their own
// Context and never read another thread's board.
struct SplitPoint
{
    Position position;
    MoveOrdering::SortableMoveList moves;
    int depthRemaining = 0;
    int ply = 0;

    std::atomic<int> alpha{0};
    std::atomic<int> beta{0};
    std::atomic<SplitBest> best{};
    std::atomic<int> pending{0}; // unfinished help tasks
    std::atomic<bool> cutoff{false};
    std::atomic<bool> aborted{false};

    SplitPoint(const Position& position, const MoveOrdering::SortableMoveList& moves,
               int depthRemaining, int ply, int alpha, int beta, int bestScore,
               int bestMoveIndex) noexcept
        : position(position), moves(moves), depthRemaining(depthRemaining), ply(ply), alpha{alpha},
          beta{beta}, best{SplitBest{bestScore, bestMoveIndex}}, pending{0}, cutoff{false},
          aborted{false}
    {
    }

    SplitPoint(const SplitPoint&) = delete;
    SplitPoint& operator=(const SplitPoint&) = delete;

    [[nodiscard]] int bestMoveIndex() const noexcept
    {
        return best.load(std::memory_order_acquire).moveIndex;
    }

    [[nodiscard]] int bestScore() const noexcept
    {
        return best.load(std::memory_order_acquire).score;
    }
};

// Call only from ROOT/MAIN after the eldest child has returned with no abort
// and no cutoff. Quiescence must not split.
[[nodiscard]] inline bool canSplit(int numWorkers, size_t moveCount, int depthRemaining,
                                   bool aborted) noexcept
{
    return numWorkers > 1 && moveCount > 1 && depthRemaining >= MIN_SPLIT_DEPTH && !aborted;
}

// Increment `pending` at submit; this guard decrements when the task exits
// (search, cutoff, or abort). Without the increment-before-submit, the
// splitter can observe pending == 0 and return while tasks are still queued.
struct SplitPendingGuard
{
    std::atomic<int>& pending;

    explicit SplitPendingGuard(std::atomic<int>& pending) noexcept : pending(pending) {}
    ~SplitPendingGuard() { pending.fetch_sub(1, std::memory_order_acq_rel); }

    SplitPendingGuard(const SplitPendingGuard&) = delete;
    SplitPendingGuard& operator=(const SplitPendingGuard&) = delete;
};

[[nodiscard]] inline bool splitShouldStop(const SplitPoint& split) noexcept
{
    return split.cutoff.load(std::memory_order_acquire) ||
           split.aborted.load(std::memory_order_acquire);
}

// Publish a child score into the shared window. White raises alpha; Black
// lowers beta. Returns true if the split is now a cutoff.
template <Color color>
inline bool applySplitScore(SplitPoint& split, int score, int moveIndex) noexcept
{
    constexpr bool isMaximizing = color == Color::WHITE;

    if constexpr (isMaximizing)
    {
        int curAlpha = split.alpha.load(std::memory_order_relaxed);
        while (score > curAlpha)
        {
            if (split.alpha.compare_exchange_weak(curAlpha, score, std::memory_order_relaxed,
                                                  std::memory_order_relaxed))
            {
                break;
            }
        }

        SplitBest curBest = split.best.load(std::memory_order_relaxed);
        const SplitBest nextBest{score, moveIndex};
        while (score > curBest.score)
        {
            if (split.best.compare_exchange_weak(curBest, nextBest, std::memory_order_relaxed))
            {
                break;
            }
        }
    }
    else
    {
        int curBeta = split.beta.load(std::memory_order_relaxed);
        while (score < curBeta)
        {
            if (split.beta.compare_exchange_weak(curBeta, score, std::memory_order_relaxed,
                                                 std::memory_order_relaxed))
            {
                break;
            }
        }

        SplitBest curBest = split.best.load(std::memory_order_relaxed);
        const SplitBest nextBest{score, moveIndex};
        while (score < curBest.score)
        {
            if (split.best.compare_exchange_weak(curBest, nextBest, std::memory_order_relaxed))
            {
                break;
            }
        }
    }

    if (split.beta.load(std::memory_order_relaxed) <= split.alpha.load(std::memory_order_relaxed))
    {
        split.cutoff.store(true, std::memory_order_release);
        return true;
    }
    return false;
}
