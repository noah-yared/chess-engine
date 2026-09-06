#pragma once

#include <algorithm>
#include <optional>
#include <random>

#include "board/bitboards.h"
#include "board/board_state.h"
#include "board/zobrist.h"
#include "move/move.h"
#include "move/piece_square_deltas.h"
#include "util/static_vector.h"

template <typename RNG>
class ZobristHasher
{
  public:
    // explicit ZobristHasher(const Zobrist<RNG>& zobrist): zobrist_{zobrist} {};

    static constexpr u64 initialZobristHash() noexcept
    {
        u64 hash = 0ull;
        // apply piece keys
        for (int bbKey = 0; bbKey < NUM_BITBOARDS; bbKey++)
            for (int square = 0; square < SQUARES; square++)
                if (STARTING_BBS[bbKey] & (1ULL << square))
                    hash ^= zobrist_.pieceKeys[bbKey][square];
        // apply castling keys
        hash ^= zobrist_.castlingKeys[STARTING_CASTLE_BITS];
        return hash;
    }

    static u64 computeZobristHash(const Bitboards& bitboards, const BoardState state) noexcept
    {
        u64 hash = 0ull;
        // apply piece keys
        for (int bbKey = 0; bbKey < NUM_BITBOARDS; bbKey++)
            for (int square = 0; square < SQUARES; square++)
                if (bitboards.bb(bbKey) & (1ULL << square))
                    hash ^= zobrist_.pieceKeys[bbKey][square];
        // apply castling keys
        hash ^= zobrist_.castlingKeys[state.castlingBits()];
        // apply enpassant keys
        if (state.getEnpassantSquare())
            hash ^= zobrist_.enpassantKeys[*state.getEnpassantSquare() % RANKS];
        // apply turn key
        if (state.blackToMove())
            hash ^= zobrist_.turnKey;
        return hash;
    }

    template <MoveType mType, typename Container>
    static u64 getHashUpdateMask(const Move move, const Container& deltas,
                                 const std::optional<int> maybePreviousEnpassantSq,
                                 int oldCastlingBits, int newCastlingBits) noexcept
    {
        return pieceSquareDeltasMask(deltas) xor
               castlingPrivilegesMask<mType>(oldCastlingBits, newCastlingBits) xor
               enpassantSquareMask<mType>(move, maybePreviousEnpassantSq) xor turnMask<mType>();
    }

  private:
    static inline Zobrist<RNG> zobrist_{};

    template <typename Container>
    static inline u64 pieceSquareDeltasMask(const Container& deltas) noexcept
    {
        u64 mask = 0ULL;
        for (const auto [bbKey, square] : deltas)
            mask ^= zobrist_.pieceKeys[bbKey][square];
        return mask;
    }

    template <MoveType mType>
    static inline u64 enpassantSquareMask(const Move move,
                                          std::optional<int> maybePreviousEnpassantSq) noexcept
    {
        if constexpr (mType == MoveType::DoublePawnPush)
        {
            return (maybePreviousEnpassantSq
                        ? zobrist_.enpassantKeys[*maybePreviousEnpassantSq % RANKS]
                        : 0ULL) xor
                   zobrist_.enpassantKeys[move.enpassantTargetSquare() % RANKS];
        }
        else
        {
            return maybePreviousEnpassantSq
                       ? zobrist_.enpassantKeys[*maybePreviousEnpassantSq % RANKS]
                       : 0ULL;
        }
    }

    template <MoveType mType>
    static inline u64 castlingPrivilegesMask(int oldCastlingBits, int newCastlingBits) noexcept
    {
        return zobrist_.castlingKeys[oldCastlingBits] ^ zobrist_.castlingKeys[newCastlingBits];
    }

    template <MoveType mType>
    static inline u64 turnMask() noexcept
    {
        return zobrist_.turnKey;
    }
};
