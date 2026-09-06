#pragma once

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <optional>
#include <vector>

#include "board/constants.h"
#include "board/position.h"
#include "move/move.h"
#include "move/move_generator.h"
#include "search/move_ordering.h"
#include "search/search_types.h"
#include "search/state_stack.h"
#include "search/transposition_table.h"
#include "concurrency/thread_pool.h"
#include "search/split_point.h"

class Searcher
{
  public:
    // Preconditions:
    // - root has at least one legal move.
    // - config.limits.maxDepth >= 1.
    static SearchResult search(const Position& root, const SearchConfig& config,
                               TranspositionTable* tt, ThreadPool* threadPool = nullptr)
    {
        // initialize search contexts for each threadpool worker
        const int n = threadPool != nullptr ? threadPool->numWorkers() : 1;
        std::vector<Context> contexts;
        contexts.reserve(n);
        for (int i = 0; i < n; ++i) {
            contexts.emplace_back(root, config, config.options.useTT ? tt : nullptr);
        }

        // ensure valid max depth
        assert(config.limits.maxDepth >= 1 && "maxDepth must be >= 1!");

        // apply iterative deepening
        std::optional<SearchResult> lastCompleted;

        for (int depth = 1; depth <= config.limits.maxDepth; ++depth)
        {
            auto newResult = root.isWhiteToMove() ? searchRoot<Color::WHITE>(root, contexts, depth, threadPool)
                                                  : searchRoot<Color::BLACK>(root, contexts, depth, threadPool);
            if (newResult.aborted)
                break; // ran out of search time

            lastCompleted = newResult;
        }

        if (!lastCompleted.has_value())
        {
            assert(false && "Search failed before completing depth 1!");
            std::abort();
        }
        return *lastCompleted;
    }

  private:
    enum class Phase
    {
        ROOT,
        MAIN,
        QUIESCENCE,
    };

    struct NodeResult
    {
        int score = 0;
        bool aborted = false;
    };

    class Timer
    {
      public:
        Timer() = delete;
        explicit Timer(int durationMS)
            : deadline_{std::chrono::steady_clock::now() + std::chrono::milliseconds(durationMS)},
              numPolls_{0}
        {
        }

        [[nodiscard]] bool isExpired()
        {
            if (++numPolls_ % ABORT_CHECK_PERIOD != 0)
            {
                return false;
            }
            return std::chrono::steady_clock::now() >= deadline_;
        }

      private:
        std::chrono::steady_clock::time_point deadline_;
        u64 numPolls_;
    };

    struct Context
    {
        Position position;
        SearchStats stats;
        StateStack<BoardStateSnapshot> undoStack;
        MoveList moveBuffer;
        TranspositionTable* tt = nullptr;
        const SearchConfig* config = nullptr;
        std::optional<Timer> timer;

        Context(const Position& position, const SearchConfig& config, TranspositionTable* tt)
            : position(position), stats(), undoStack(), moveBuffer(), tt(tt), config(&config)
        {
            // add deadline for search if time control enabled
            if (config.options.useTimeManagement)
            {
                timer.emplace(config.limits.timeLimitMS);
            }
            else
            {
                timer = std::nullopt;
            }
        }
    };

    static void advance(Context& context, Move move) noexcept
    {
        context.undoStack.push(context.position.getStateSnapshot());
        context.position.applyMove(move);
    }

    static void backtrack(Context& context, Move move) noexcept
    {
        context.position.undoMove(move, context.undoStack.pop());
    }

    template <Color color>
    static void generateSortedMoves(MoveOrdering::SortableMoveList& sortableMoveBuffer,
                                    Context& context, Phase phase)
    {
        context.moveBuffer.clear();
        if (phase == Phase::QUIESCENCE)
        {
            MoveGenerator::pushNoisyMoves<color>(context.position, context.moveBuffer);
        }
        else
        {
            MoveGenerator::pushLegalMoves<color>(context.position, context.moveBuffer);
        }

        MoveOrdering::orderMoves(sortableMoveBuffer, context.position, context.moveBuffer,
                                 context.tt, phase == Phase::QUIESCENCE);
    }

    template <Color color>
    static int terminalScore(Context& context, int ply)
    {
        if (MoveGenerator::isKingInCheck<color>(context.position)) // checkmate
        { // in checkmate; penalize mate score by distance to mate
            if constexpr (color == Color::WHITE)
            {
                // black wins
                return -(MATE_SCORE - MATE_DEPTH_PENALTY * ply);
            }
            else
            {
                // white wins
                return MATE_SCORE - MATE_DEPTH_PENALTY * ply;
            }
        }
        else // in stalemate
        {
            return STALEMATE_SCORE;
        }
    }

    [[nodiscard]] static Bound getBound(int score, int alpha, int beta) noexcept
    {
        if (score < alpha)
            return Bound::UPPER;
        if (score > beta)
            return Bound::LOWER;
        return Bound::EXACT;
    }

    static void storeEvalIntoTT(Context& context, int score, int depth, int alpha, int beta,
                                Move bestMove)
    {
        if (context.tt == nullptr)
            return;

        context.tt->store(context.position.getHash(), score, depth, getBound(score, alpha, beta),
                          bestMove.orderingKey());
    }

    // `color` is the side to move at the parent node. Searches `move` and restores.
    template <Color color>
    static NodeResult searchChild(std::vector<Context>& contexts, Context& context, Move move,
                                  int alpha, int beta, int childDepth, int childPly,
                                  ThreadPool* threadPool)
    {
        advance(context, move);
        auto result =
            alphaBeta<opposite<color>()>(contexts, alpha, beta, childDepth, childPly, threadPool);
        backtrack(context, move);
        return result;
    }

    // Applies the score to the alpha/beta bounds and updates the best move index
    // if the score is better than the current alpha/beta bounds.
    // Returns true if a cutoff occurred.
    template <Color color>
    static bool applyLocalScore(int score, int& alpha, int& beta, int& bestMoveIndex,
                                int moveIndex) noexcept
    {
        if constexpr (color == Color::WHITE)
        {
            if (score > alpha)
            {
                alpha = score;
                bestMoveIndex = moveIndex;
            }
        }
        else
        {
            if (score < beta)
            {
                beta = score;
                bestMoveIndex = moveIndex;
            }
        }
        return beta <= alpha;
    }

    template <Color color>
    static void executeSplitMove(SplitPoint& splitPoint, std::vector<Context>& contexts,
                                 int moveIndex, ThreadPool* threadPool)
    {
        SplitPendingGuard guard(splitPoint.pending);
        if (splitShouldStop(splitPoint))
            return;

        const auto move = splitPoint.moves.moves[moveIndex].move;
        Context& context = contexts[ThreadPool::workerId()];
        Position saved = context.position;
        context.position = splitPoint.position;

        auto result = searchChild<color>(
            contexts, context, move, splitPoint.alpha.load(std::memory_order_relaxed),
            splitPoint.beta.load(std::memory_order_relaxed), splitPoint.depthRemaining - 1,
            splitPoint.ply + 1, threadPool);

        context.position = saved;

        if (result.aborted)
        {
            splitPoint.aborted.store(true, std::memory_order_relaxed);
            return;
        }

        applySplitScore<color>(splitPoint, result.score, moveIndex);
    }

    template <Color color>
    static NodeResult searchYoungBrothersSerial(std::vector<Context>& contexts, Context& context,
                                                const MoveOrdering::SortableMoveList& sortedMoves,
                                                int& alpha, int& beta, int& bestMoveIndex,
                                                int depthRemaining, int ply, ThreadPool* threadPool)
    {
        for (int i = 1; i < static_cast<int>(sortedMoves.count); ++i)
        {
            auto result =
                searchChild<color>(contexts, context, sortedMoves.moves[i].move, alpha, beta,
                                   depthRemaining - 1, ply + 1, threadPool);
            if (result.aborted)
                return result;
            if (applyLocalScore<color>(result.score, alpha, beta, bestMoveIndex, i))
                break;
        }
        return NodeResult{.score = color == Color::WHITE ? alpha : beta};
    }

    template <Color color>
    static NodeResult searchYoungBrothersParallel(std::vector<Context>& contexts, Context& context,
                                                  const MoveOrdering::SortableMoveList& sortedMoves,
                                                  int& alpha, int& beta, int& bestMoveIndex,
                                                  int depthRemaining, int ply,
                                                  ThreadPool* threadPool)
    {
        constexpr bool isMaximizingPlayer = color == Color::WHITE;
        SplitPoint splitPoint(context.position, sortedMoves, depthRemaining, ply, alpha, beta,
                              isMaximizingPlayer ? alpha : beta, /*bestMoveIndex=*/0);

        for (int moveIndex = 1; moveIndex < static_cast<int>(sortedMoves.count); ++moveIndex)
        {
            splitPoint.pending.fetch_add(1);
            threadPool->submit(
                [&splitPoint, &contexts, moveIndex, threadPool]
                { executeSplitMove<color>(splitPoint, contexts, moveIndex, threadPool); });
        }

        while (splitPoint.pending.load() > 0)
            threadPool->tryRunOne();

        if (splitPoint.aborted.load(std::memory_order_relaxed))
            return {.aborted = true};

        const int bestScore = splitPoint.bestScore();
        if constexpr (isMaximizingPlayer)
        {
            if (bestScore > alpha)
                alpha = bestScore;
        }
        else
        {
            if (bestScore < beta)
                beta = bestScore;
        }
        bestMoveIndex = splitPoint.bestMoveIndex();
        return NodeResult{.score = isMaximizingPlayer ? alpha : beta};
    }

    template <Color color>
    static NodeResult alphaBeta(std::vector<Context>& contexts, int alpha, int beta, int depthRemaining, int ply, ThreadPool* threadPool)
    {
        const int index = threadPool != nullptr ? ThreadPool::workerId() : 0;
        auto& context = contexts[index];

        if (context.config->options.useTimeManagement && context.timer->isExpired())
        {
            return NodeResult{.aborted = true};
        }

        context.stats.nodesSearched++;

        if (depthRemaining == 0)
        {
            if (context.config->options.useQuiescence)
                return quiesce<color>(context, alpha, beta, ply);

            return {.score = context.position.evaluation()};
        }

        MoveOrdering::SortableMoveList sortedMoves;
        generateSortedMoves<color>(sortedMoves, context, Phase::MAIN);

        if (sortedMoves.count == 0)
        {
            return {.score = terminalScore<color>(context, ply)};
        }

        int currentBestMoveIndex = 0;
        int originalAlpha = alpha, originalBeta = beta;
        constexpr bool isMaximizingPlayer = color == Color::WHITE;

        auto result = searchChild<color>(contexts, context, sortedMoves.moves[0].move, alpha, beta,
                                         depthRemaining - 1, ply + 1, threadPool);
        if (result.aborted)
            return result;

        if (applyLocalScore<color>(result.score, alpha, beta, currentBestMoveIndex, 0))
        {
            if constexpr (isMaximizingPlayer)
                storeEvalIntoTT(context, alpha, depthRemaining, originalAlpha, originalBeta,
                                sortedMoves.moves[0].move);
            else
                storeEvalIntoTT(context, beta, depthRemaining, originalAlpha, originalBeta,
                                sortedMoves.moves[0].move);

            return NodeResult{.score = isMaximizingPlayer ? alpha : beta};
        }

        const bool shouldSplit = threadPool != nullptr &&
                                 canSplit(threadPool->numWorkers(), sortedMoves.count,
                                          depthRemaining, false);

        result = shouldSplit ? searchYoungBrothersParallel<color>(
                                   contexts, context, sortedMoves, alpha, beta, currentBestMoveIndex,
                                   depthRemaining, ply, threadPool)
                             : searchYoungBrothersSerial<color>(
                                   contexts, context, sortedMoves, alpha, beta, currentBestMoveIndex,
                                   depthRemaining, ply, threadPool);
        if (result.aborted)
            return result;

        if constexpr (isMaximizingPlayer)
            storeEvalIntoTT(context, alpha, depthRemaining, originalAlpha, originalBeta,
                            sortedMoves.moves[currentBestMoveIndex].move);
        else
            storeEvalIntoTT(context, beta, depthRemaining, originalAlpha, originalBeta,
                            sortedMoves.moves[currentBestMoveIndex].move);

        return NodeResult{.score = isMaximizingPlayer ? alpha : beta};
    }

    template <Color color>
    static NodeResult quiesce(Context& context, int alpha, int beta, int ply)
    {
        if (context.config->options.useTimeManagement && context.timer->isExpired())
        {
            return NodeResult{.aborted = true};
        }

        // context.stats.nodesSearched++;

        // standing pat (https://www.chessprogramming.org/Quiescence_Search)
        int bestValue = context.position.evaluation();

        if constexpr (color == Color::WHITE)
        {
            if (bestValue >= beta)
                return NodeResult{.score = bestValue};
            if (bestValue > alpha)
                alpha = bestValue;
        }
        else
        {
            if (bestValue <= alpha)
                return NodeResult{.score = bestValue};
            if (bestValue < beta)
                beta = bestValue;
        }

        MoveOrdering::SortableMoveList sortedMoves;
        generateSortedMoves<color>(sortedMoves, context, Phase::QUIESCENCE);

        for (int i = 0; i < sortedMoves.count; ++i)
        {
            const auto move = sortedMoves.moves[i].move;

            advance(context, move);
            auto result = quiesce<opposite<color>()>(context, alpha, beta, ply + 1);
            backtrack(context, move);

            if (result.aborted)
            {
                return result; // child search node aborted
            }

            int score = result.score;
            if constexpr (color == Color::WHITE)
            {
                if (score >= beta)
                    return NodeResult{.score = score};
                if (score > bestValue)
                    bestValue = score;
                if (score > alpha)
                    alpha = score;
            }
            else
            {
                if (score <= alpha)
                    return NodeResult{.score = score};
                if (score < bestValue)
                    bestValue = score;
                if (score < beta)
                    beta = score;
            }
        }

        return NodeResult{.score = bestValue};
    }

    static SearchStats accumulateStats(const std::vector<Context>& contexts) noexcept
    {
        SearchStats total;
        for (const auto& context : contexts)
            total += context.stats;
        return total;
    }

    template <Color color>
    static SearchResult searchRoot(const Position&, std::vector<Context>& contexts, int depth,
                                   ThreadPool* threadPool)
    {
        const int index = threadPool != nullptr ? ThreadPool::workerId() : 0;
        auto& context = contexts[index];

        MoveOrdering::SortableMoveList sortedMoves;
        generateSortedMoves<color>(sortedMoves, context, Phase::ROOT);
        assert(sortedMoves.count > 0 && "No move was found in searchRoot!");

        int alpha = NEGINF;
        int beta = POSINF;
        int bestMoveIndex = 0;
        const int ply = 0;

        auto result = searchChild<color>(contexts, context, sortedMoves.moves[0].move, alpha, beta,
                                         depth - 1, ply + 1, threadPool);
        if (result.aborted)
            return SearchResult{.aborted = true};

        if (applyLocalScore<color>(result.score, alpha, beta, bestMoveIndex, 0))
        {
            return SearchResult{.bestMove = sortedMoves.moves[0].move,
                                .score = color == Color::WHITE ? alpha : beta,
                                .stats = accumulateStats(contexts)};
        }

        const bool shouldSplit = threadPool != nullptr &&
                                 canSplit(threadPool->numWorkers(), sortedMoves.count, depth, false);

        result = shouldSplit ? searchYoungBrothersParallel<color>(
                                   contexts, context, sortedMoves, alpha, beta, bestMoveIndex,
                                   depth, ply, threadPool)
                             : searchYoungBrothersSerial<color>(
                                   contexts, context, sortedMoves, alpha, beta, bestMoveIndex,
                                   depth, ply, threadPool);
        if (result.aborted)
            return SearchResult{.aborted = true};

        return SearchResult{.bestMove = sortedMoves.moves[bestMoveIndex].move,
                            .score = color == Color::WHITE ? alpha : beta,
                            .stats = accumulateStats(contexts)};
    }
};
