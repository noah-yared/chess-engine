#pragma once

#include <algorithm>
#include <array>

#include "board/constants.h"
#include "board/position.h"
#include "move/move.h"
#include "move/move_list.h"
#include "search/transposition_table.h"

class MoveOrdering
{
  public:
    struct Entry
    {
        MoveVariant move;
        int score = 0;
    };
    struct SortableMoveList
    {
        std::array<Entry, MAX_POSSIBLE_LEGAL_MOVES> moves;
        size_t count = 0;
    };

    static void orderMoves(SortableMoveList& sortableMoves, const Position& position,
                           const MoveList& moves, const TranspositionTable* tt, bool inQuiescence)
    {
        const auto ttMove = getTTMove(position, tt);
        const auto moveCount = moves.size();

        sortableMoves.count = moveCount;

        for (size_t i = 0; i < moveCount; ++i)
        {
            const auto& move = moves[i];
            sortableMoves.moves[i] = Entry{move, score(move, ttMove, inQuiescence)};
        }

        std::sort(sortableMoves.moves.begin(), sortableMoves.moves.begin() + moveCount,
                  [](const Entry& lhs, const Entry& rhs) { return lhs.score > rhs.score; });
    }

  private:
    static constexpr int TT_MOVE_SCORE = 1'000'000;
    static constexpr int PROMOTION_SCORE = 100'000;
    static constexpr int CAPTURE_SCORE = 10'000;

    static constexpr std::array<std::array<int, NUM_PIECE_TYPES + 1>, NUM_PIECE_TYPES + 1> MVVLVA{{
        {15, 12, 14, 13, 11, 10, 0}, // victim P, attacker P, R, N, B, Q, K, None
        {45, 42, 44, 43, 41, 40, 0}, // victim R, attacker P, R, N, B, Q, K, None
        {25, 22, 24, 23, 21, 20, 0}, // victim N, attacker P, R, N, B, Q, K, None
        {35, 32, 34, 33, 31, 30, 0}, // victim B, attacker P, R, N, B, Q, K, None
        {55, 52, 54, 53, 51, 50, 0}, // victim Q, attacker P, R, N, B, Q, K, None
        {0, 0, 0, 0, 0, 0, 0},       // victim K, attacker P, R, N, B, Q, K, None
        {0, 0, 0, 0, 0, 0, 0},       // victim None, attacker P, R, N, B, Q, K, None
    }};

    template <MoveType mType>
    [[nodiscard]] static int tacticalScore(const Move<mType>& move, bool inQuiescence) noexcept
    {
        (void)inQuiescence;

        int score = 0;
        if constexpr (mType == MoveType::Promotion)
        {
            score += PROMOTION_SCORE;
        }

        if constexpr (CanCapture<mType>)
        {
            score += move.captured() != PieceType::NONE
                         ? CAPTURE_SCORE + MVVLVA[static_cast<size_t>(move.captured())]
                                                 [static_cast<size_t>(move.moved())]
                         : 0;
        }

        return score;
    }

    [[nodiscard]] static std::pair<int, int> step(const MoveVariant& move) noexcept
    {
        return std::visit([](const auto& arg) noexcept -> std::pair<int, int>
                          { return {arg.start(), arg.end()}; }, move);
    }

    static bool step_eq(const MoveVariant& move, std::pair<int, int> step) noexcept
    {
        return std::visit([step](const auto& arg) noexcept -> bool
                          { return arg.start() == step.first && arg.end() == step.second; }, move);
    }

    [[nodiscard]] static int score(const MoveVariant& move, std::pair<int, int> ttMove,
                                   bool inQuiescence) noexcept
    {
        if (ttMove != NullStep && step_eq(move, ttMove))
        {
            return TT_MOVE_SCORE;
        }

        return std::visit([inQuiescence](const auto& arg) noexcept
                          { return tacticalScore(arg, inQuiescence); }, move);
    }

    static constexpr std::pair<int, int> NullStep{-1, -1};

    [[nodiscard]] static std::pair<int, int> getTTMove(const Position& position,
                                                       const TranspositionTable* tt) noexcept
    {
        if (tt == nullptr)
        {
            return NullStep;
        }

        if (auto maybeEntry = tt->probe(position.getHash()); maybeEntry)
        {
            auto& ttEntry = *(*maybeEntry);
            if (ttEntry.hasAtLeastDepth(1))
            {
                return ttEntry.getMove();
            }
        }
        return NullStep;
    }
};
