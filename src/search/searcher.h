#pragma once

#include <cassert>
#include <optional>

#include "board/constants.h"
#include "board/position.h"
#include "move/move.h"
#include "move/move_generator.h"
#include "search/move_ordering.h"
#include "search/search_types.h"
#include "search/state_stack.h"
#include "search/transposition_table.h"

class Searcher
{
  public:
    static SearchResult search(const Position& root, const SearchConfig& config,
                               TranspositionTable* tt)
    {
        // initialize search context
        Context context(root, config, tt);

        // ensure valid max depth
        assert(context.config->limits.maxDepth >= 1 && "maxDepth must be >= 1!");

        // apply iterative deepening
        std::optional<SearchResult> lastCompleted;

        for (int depth = 1; depth <= context.config->limits.maxDepth; ++depth)
        {
            auto newResult = root.isWhiteToMove() ? searchRoot<Color::WHITE>(root, context, depth)
                                                  : searchRoot<Color::BLACK>(root, context, depth);
            if (newResult.aborted)
            {
                break; // ran out of search time
            }

            lastCompleted = newResult;
        }

        assert(lastCompleted.has_value());
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

    struct Context
    {
        Position position;
        SearchStats stats;
        StateStack<BoardStateSnapshot> undoStack;
        MoveList moveBuffer;
        TranspositionTable* tt = nullptr;
        const SearchConfig* config = nullptr;

        Context(const Position& position, const SearchConfig& config, TranspositionTable* tt)
            : position(position), stats(), undoStack(), moveBuffer(), tt(tt), config(&config)
        {
        }
    };

    static void advance(Context& context, MoveVariant variant) noexcept
    {
        context.undoStack.push(context.position.getStateSnapshot());
        std::visit([&context](auto&& move) noexcept
                   { context.position.applyMove<std::decay_t<decltype(move)>::type>(move); },
                   variant);
    }

    static void backtrack(Context& context, MoveVariant variant) noexcept
    {
        std::visit(
            [&context](auto&& move) noexcept
            {
                context.position.undoMove<std::decay_t<decltype(move)>::type>(
                    move, context.undoStack.pop());
            },
            variant);
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

    [[nodiscard]] static std::pair<int, int> getStep(MoveVariant move) noexcept
    {
        return std::visit([](auto&& move) noexcept -> std::pair<int, int>
                          { return {move.start(), move.end()}; }, move);
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
                                MoveVariant bestMove)
    {
        if (context.tt == nullptr)
            return;

        context.tt->store(context.position.getHash(), score, depth, getBound(score, alpha, beta),
                          getStep(bestMove));
    }

    template <Color color>
    static NodeResult alphaBeta(Context& context, int alpha, int beta, int depthRemaining, int ply)
    {
        context.stats.nodesSearched++;

        if (depthRemaining == 0)
        {
            return quiesce<color>(context, alpha, beta, ply);
        }

        MoveOrdering::SortableMoveList sortedMoves;
        generateSortedMoves<color>(sortedMoves, context, Phase::MAIN);

        if (sortedMoves.count == 0)
        {
            return {.score = terminalScore<color>(context, ply)};
        }

        MoveVariant currentBestMove = sortedMoves.moves[0].move;
        int originalAlpha = alpha, originalBeta = beta;
        constexpr bool isMaximizingPlayer = color == Color::WHITE;

        for (int i = 0; i < sortedMoves.count; ++i)
        {
            const auto move = sortedMoves.moves[i].move;

            advance(context, move);
            int score =
                alphaBeta<opposite<color>()>(context, alpha, beta, depthRemaining - 1, ply + 1)
                    .score;
            backtrack(context, move);

            if constexpr (isMaximizingPlayer)
            {
                if (score > alpha)
                {
                    alpha = score;
                    currentBestMove = move;
                }
            }
            else
            {
                if (score < beta)
                {
                    beta = score;
                    currentBestMove = move;
                }
            }

            if (beta <= alpha)
            {
                break;
            }
        }

        if constexpr (isMaximizingPlayer)
            storeEvalIntoTT(context, alpha, ply, originalAlpha, originalBeta, currentBestMove);
        else
            storeEvalIntoTT(context, beta, ply, originalAlpha, originalBeta, currentBestMove);

        return NodeResult{.score = isMaximizingPlayer ? alpha : beta};
    }

    template <Color color>
    static NodeResult quiesce(Context& context, int alpha, int beta, int ply)
    {
        // context.stats.nodesSearched++;

        // standing pat (https://www.chessprogramming.org/Quiescence_Search)
        int bestValue = context.position.evaluation();

        if constexpr (color == Color::WHITE)
        {
            if (bestValue >= beta)
            {
                return NodeResult{.score = bestValue};
            }
            if (bestValue > alpha)
            {
                alpha = bestValue;
            }
        }
        else
        {
            if (bestValue <= alpha)
            {
                return NodeResult{.score = bestValue};
            }
            if (bestValue < beta)
            {
                beta = bestValue;
            }
        }

        MoveOrdering::SortableMoveList sortedMoves;
        generateSortedMoves<color>(sortedMoves, context, Phase::QUIESCENCE);

        for (int i = 0; i < sortedMoves.count; ++i)
        {
            const auto move = sortedMoves.moves[i].move;

            advance(context, move);
            int score = quiesce<opposite<color>()>(context, alpha, beta, ply + 1).score;
            backtrack(context, move);

            if constexpr (color == Color::WHITE)
            {
                if (score >= beta)
                {
                    return NodeResult{.score = score};
                }
                if (score > bestValue)
                {
                    bestValue = score;
                }
                if (score > alpha)
                {
                    alpha = score;
                }
            }
            else
            {
                if (score <= alpha)
                {
                    return NodeResult{.score = score};
                }
                if (score < bestValue)
                {
                    bestValue = score;
                }
                if (score < beta)
                {
                    beta = score;
                }
            }
        }

        return NodeResult{.score = bestValue};
    }

    template <Color color>
    static SearchResult searchRoot(const Position& root, Context& context, int depth)
    {
        std::optional<MoveVariant> bestMove = std::nullopt;
        int bestScore = root.isWhiteToMove() ? NEGINF : POSINF;

        MoveOrdering::SortableMoveList sortedMoves;
        generateSortedMoves<color>(sortedMoves, context, Phase::ROOT);

        for (int i = 0; i < sortedMoves.count; ++i)
        {
            const auto move = sortedMoves.moves[i].move;

            advance(context, move);
            int score = alphaBeta<opposite<color>()>(context, NEGINF, POSINF, depth - 1, 1).score;
            backtrack(context, move);

            if (root.isWhiteToMove())
            {
                if (score >= bestScore)
                {
                    bestScore = score;
                    bestMove = move;
                }
            }
            else
            {
                if (score <= bestScore)
                {
                    bestScore = score;
                    bestMove = move;
                }
            }
        }

        assert(bestMove.has_value() && "No move was found in searchRoot!");
        return SearchResult{
            .bestMove = bestMove.value(), .score = bestScore, .stats = context.stats};
    }
};
