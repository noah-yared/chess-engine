#pragma once

#include <algorithm>
#include <optional>
#include <random>

#include "bitboards.h"
#include "board_state.h"
#include "delta.h"
#include "move.h"
#include "static_vector.h"
#include "zobrist.h"

template<typename RNG>
class ZobristHasher {
public:
  // explicit ZobristHasher(const Zobrist<RNG>& zobrist): zobrist_{zobrist} {};

  static constexpr u64 initialZobristHash() {
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

  static u64 computeZobristHash(const Bitboards& bitboards, const BoardState state) {
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

  template<MoveType mType, typename Container>
  static u64 getHashUpdateMask(const Move<mType> move, const Container& deltas, const std::optional<int> maybePreviousEnpassantSq, int oldCastlingBits, int newCastlingBits) {
    return pieceSquareDeltasMask(deltas)
       xor castlingPrivilegesMask<mType>(oldCastlingBits, newCastlingBits)
       xor enpassantSquareMask<mType>(move, maybePreviousEnpassantSq)
       xor turnMask<mType>();
  }

private:
  static inline Zobrist<RNG> zobrist_{};

  template<typename Container>
  static inline u64 pieceSquareDeltasMask(const Container& deltas) {
    u64 mask = 0ULL;
    for (const auto [bbKey, square] : deltas)
      mask ^= zobrist_.pieceKeys[bbKey][square]; 
    return mask;
  }

  template<MoveType mType>
  static inline u64 enpassantSquareMask(const Move<mType> move, std::optional<int> maybePreviousEnpassantSq) {
    if constexpr(mType == MoveType::DoublePawnPush) {
      return (maybePreviousEnpassantSq ? zobrist_.enpassantKeys[*maybePreviousEnpassantSq % RANKS] : 0ULL)
         xor zobrist_.enpassantKeys[((move.start() + move.end()) / 2) % RANKS];
    } else {
      return maybePreviousEnpassantSq ? zobrist_.enpassantKeys[*maybePreviousEnpassantSq % RANKS] : 0ULL;
    }
  }

  template<MoveType mType>
  static inline u64 castlingPrivilegesMask(int oldCastlingBits, int newCastlingBits) {
    if constexpr (mType == MoveType::Castle) {
      return zobrist_.castlingKeys[oldCastlingBits] ^ zobrist_.castlingKeys[newCastlingBits];
    } else {
      return 0ULL;
    }
  }

  template<MoveType mType>
  static inline u64 turnMask() {
    return zobrist_.turnKey;
  }
};
