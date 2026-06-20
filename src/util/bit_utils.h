#pragma once

#include <array>
#include <bit>
#include <concepts>
#include <type_traits>

#include "board/directions.h"
#include "util/platform.h"

// helper concepts to ensure valid file/rank values
template <char file>
concept IsFile = ('a' <= file) && (file <= 'h');
template <int rank>
concept IsRank = (1 <= rank) && (rank <= 8);

class BitUtils
{
  public:
    static constexpr inline u64 stepBitsForward(u64 bb, Direction dir, int numSteps = 1) noexcept
    {
        return sft(bb, numSteps * Directions::sfamt(dir));
    }

    static constexpr inline int ctz(u64 bb) noexcept { return std::countr_zero(bb); }
    static constexpr inline int clz(u64 bb) noexcept { return std::countl_zero(bb); }
    static constexpr inline int popcount(u64 bb) noexcept { return std::popcount(bb); }
    static constexpr inline int popBit(u64& bb) noexcept
    {
        int popped = ctz(bb); // get lsb index
        bb &= bb - 1;         // clear lsb
        return popped;
    }
    template <typename ResultType, typename Accumulator>
        requires std::regular_invocable<Accumulator, ResultType, int> &&
                 std::is_default_constructible_v<ResultType>
    static constexpr inline ResultType accumulateBits(u64 bb, Accumulator accumulator,
                                                      ResultType init = {}) noexcept
    {
        for (int lsbIndex = popBit(bb); lsbIndex != 64; lsbIndex = popBit(bb))
            init = accumulator(init, lsbIndex);
        return init;
    }

    template <typename FuncType>
        requires std::regular_invocable<FuncType, int>
    static constexpr inline void bitsForEach(u64 bb, FuncType func) noexcept
    {
        for (int lsbIndex = popBit(bb); lsbIndex != 64; lsbIndex = popBit(bb))
            func(lsbIndex);
    }

    // general file/rank masking helpers
    template <char file>
        requires IsFile<file>
    static constexpr inline u64 clearFile(u64 bb) noexcept
    {
        return bb & ~fileMask<file>();
    }
    template <char file>
        requires IsFile<file>
    static constexpr inline u64 filterFile(u64 bb) noexcept
    {
        return bb & fileMask<file>();
    }
    template <int rank>
        requires IsRank<rank>
    static constexpr inline u64 clearRank(u64 bb) noexcept
    {
        return bb & ~rankMask<rank>();
    }
    template <int rank>
        requires IsRank<rank>
    static constexpr inline u64 filterRank(u64 bb) noexcept
    {
        return bb & rankMask<rank>();
    }

    // specific file/rank masking helpers (commonly used)
    static u64 clearFirstFile(u64 bb) noexcept { return clearFile<'a'>(bb); }
    static u64 filterFirstFile(u64 bb) noexcept { return filterFile<'a'>(bb); }
    static u64 clearLastFile(u64 bb) noexcept { return clearFile<'h'>(bb); }
    static u64 filterLastFile(u64 bb) noexcept { return filterFile<'h'>(bb); }
    static u64 clearPromotionRank(Color color, u64 bb) noexcept
    {
        return color == Color::WHITE ? clearRank<WhitePromotionRank>(bb)
                                     : clearRank<BlackPromotionRank>(bb);
    }
    static u64 filterPromotionRank(Color color, u64 bb) noexcept
    {
        return color == Color::WHITE ? filterRank<WhitePromotionRank>(bb)
                                     : filterRank<BlackPromotionRank>(bb);
    }
    static u64 clearStartingPawnRank(Color color, u64 bb) noexcept
    {
        return color == Color::WHITE ? clearRank<WhitePawnStartRank>(bb)
                                     : clearRank<BlackPawnStartRank>(bb);
    }
    static u64 filterStartingPawnRank(Color color, u64 bb) noexcept
    {
        return color == Color::WHITE ? filterRank<WhitePawnStartRank>(bb)
                                     : filterRank<BlackPawnStartRank>(bb);
    }

  private:
    // helpful constants
    static constexpr u64 LeftEdgeMask = 0x80'80'80'80'80'80'80'80ULL;
    static constexpr u64 BottomEdgeMask = 0xFFULL;
    static constexpr int WhitePromotionRank = 8;
    static constexpr int BlackPromotionRank = 1;
    static constexpr int WhitePawnStartRank = 2;
    static constexpr int BlackPawnStartRank = 7;
    static constexpr int NumFiles = 8;
    static constexpr int NumRanks = 8;

    // sfamt > 0: shift left by sfamt, sfamt < 0: shift right by -sfamt
    static constexpr inline u64 sft(u64 bb, int sfamt) noexcept
    {
        return sfamt > 0 ? bb << sfamt : bb >> -sfamt;
    }

    template <char file>
        requires IsFile<file>
    static constexpr u64 fileMask() noexcept
    {
        return LeftEdgeMask >> (file - 'a');
    }

    template <int rank>
        requires IsRank<rank>
    static constexpr u64 rankMask() noexcept
    {
        return BottomEdgeMask << (8 * (rank - 1));
    }
};
