#pragma once

#include <array>

#include "move/move.h"
#include "move/move_list.h"

class MoveOrdering
{
  private:
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
    [[nodiscard]] static int mvvlva(const Move<mType> move) noexcept
    {
        return MVVLVA[static_cast<int>(move.captured())][static_cast<int>(move.moved())];
    }

    [[nodiscard]] static int mvvlva(const MoveVariant& move) noexcept
    {
        return std::visit([](auto&& arg) noexcept { return mvvlva(arg); }, move);
    }

    static void
    sortByMvvlvaWithPriority(MoveList& moveList,
                             std::optional<MoveVariant> maybePreferredMove = std::nullopt) noexcept
    {
        if (!maybePreferredMove)
        {
            std::sort(moveList.begin(), moveList.end(), [](auto&& move1, auto&& move2) noexcept
                      { return mvvlva(move1) > mvvlva(move2); });
        }
        else
        {
            std::sort(moveList.begin(), moveList.end(),
                      [preferredMove = *maybePreferredMove](auto&& move1, auto&& move2) noexcept
                      {
                          if (move1 == preferredMove)
                              return true;
                          if (move2 == preferredMove)
                              return false;
                          return mvvlva(move1) > mvvlva(move2);
                      });
        }
    }

  public:
    static void sort(MoveList& moveList,
                     std::optional<MoveVariant> maybePreferredMove = std::nullopt) noexcept
    {
        sortByMvvlvaWithPriority(moveList, maybePreferredMove);
    }
};
