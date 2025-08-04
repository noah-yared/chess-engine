#pragma once

#include <array>
#include <bit>
#include <limits>

#include "pieces.h"
#include "platform.h"

constexpr int NEGINF = std::numeric_limits<int>::min();
constexpr int POSINF = std::numeric_limits<int>::max();

constexpr int MIN_EVAL = std::numeric_limits<i16>::min();
constexpr int MAX_EVAL = std::numeric_limits<i16>::max();

constexpr int MAX_POSSIBLE_LEGAL_MOVES = 218;

constexpr int SEARCH_DEPTH = 3;
// constexpr size_t MAX_DEPTH_ = 16;

constexpr int NUM_DIRECTIONS = 8;

constexpr int NUM_COLORS = 2;
constexpr int NUM_PIECE_TYPES = 6;
constexpr int NUM_BITBOARDS = NUM_PIECE_TYPES * NUM_COLORS;

constexpr int RANKS = 8;
constexpr int FILES = 8;
constexpr int SQUARES = RANKS * FILES;

inline constexpr int CASTLING_DESTINATION(char priv) noexcept {
  /**
   * map castling priv chars ['k', 'q', 'K', 'Q'] to their respective castling dest square,
   * i.e. [57, 61, 1, 5]
   * 
   * found hash function with ../scripts/find_perfect_hasher.py
   */
  return (((priv * 45) ^ 40) >> 2) & 63;
}

inline constexpr int PRIV_BIT_OFFSET(char priv) noexcept {
  /**
   * map castling priv chars to unique index in [0, 4)
   * 
   * found hash function with ../scripts/find_perfect_hasher.py
   */
  // 'k': h(107) = 2, 'q': h(113) = 3, 'K': h(75) = 0, 'Q': h(81) = 1
  return (priv & 48) >> 4;
}

inline constexpr std::pair<char, char> CASTLING_RIGHT_CHARS(Color c) noexcept {
  std::array<std::pair<char, char>, 2> rights = {
    std::pair{'k', 'q'},
    std::pair{'K', 'Q'},
  };
  return rights[c == Color::WHITE];
}

inline constexpr u64 QUEENSIDE_CASTLE_MASK(Color color) noexcept {
  constexpr std::array<u64, 2> queensideMasks { 0b01110000ULL << 56, 0b01110000ULL };
  return queensideMasks[color == Color::WHITE];
}

inline constexpr u64 KINGSIDE_CASTLE_MASK(Color color) noexcept {
  constexpr std::array<u64, 2> kingsideMasks { 0b0110ULL << 56, 0b0110ULL };
  return kingsideMasks[color == Color::WHITE];
}

constexpr int NUM_CASTLE_RIGHTS = 4;
constexpr int CASTLING_COMBINATIONS = 1 << NUM_CASTLE_RIGHTS;
constexpr int STARTING_CASTLE_BITS = 0xf;
constexpr int NUM_CASTLING_PRIVILEGE_STATES = 2;
constexpr int WHITE_KINGSIDE_CASTLING_RIGHTS_MASK  = 1 << PRIV_BIT_OFFSET('K');
constexpr int WHITE_QUEENSIDE_CASTLING_RIGHTS_MASK = 1 << PRIV_BIT_OFFSET('Q');
constexpr int BLACK_KINGSIDE_CASTLING_RIGHTS_MASK  = 1 << PRIV_BIT_OFFSET('k');
constexpr int BLACK_QUEENSIDE_CASTLING_RIGHTS_MASK = 1 << PRIV_BIT_OFFSET('q');

constexpr u64 LEFT_EDGE_MASK = 0x80'80'80'80'80'80'80'80ULL;
constexpr u64 RIGHT_EDGE_MASK = LEFT_EDGE_MASK >> 7;

constexpr u64 STARTING_WHITE_PAWNS = 0b1111'1111'0000'0000ULL;
constexpr u64 STARTING_BLACK_PAWNS = STARTING_WHITE_PAWNS << 40;
constexpr u64 STARTING_WHITE_ROOKS = 0b1000'0001ULL;
constexpr u64 STARTING_BLACK_ROOKS = STARTING_WHITE_ROOKS << 56;
constexpr u64 STARTING_WHITE_KNIGHTS = 0b0100'0010ULL;
constexpr u64 STARTING_BLACK_KNIGHTS = STARTING_WHITE_KNIGHTS << 56;
constexpr u64 STARTING_WHITE_BISHOPS = 0b0010'0100ULL;
constexpr u64 STARTING_BLACK_BISHOPS = STARTING_WHITE_BISHOPS << 56;
constexpr u64 STARTING_WHITE_QUEENS = 0b0001'0000ULL;
constexpr u64 STARTING_BLACK_QUEENS = STARTING_WHITE_QUEENS << 56;
constexpr u64 STARTING_WHITE_KINGS = 0b0000'1000ULL;
constexpr u64 STARTING_BLACK_KINGS = STARTING_WHITE_KINGS << 56;

constexpr std::array<u64, NUM_BITBOARDS> STARTING_BBS = {
  STARTING_BLACK_PAWNS,
  STARTING_BLACK_ROOKS,
  STARTING_BLACK_KNIGHTS,
  STARTING_BLACK_BISHOPS,
  STARTING_BLACK_QUEENS,
  STARTING_BLACK_KINGS,
  STARTING_WHITE_PAWNS,
  STARTING_WHITE_ROOKS,
  STARTING_WHITE_KNIGHTS,
  STARTING_WHITE_BISHOPS,
  STARTING_WHITE_QUEENS,
  STARTING_WHITE_KINGS
};

constexpr u64 STARTING_WHITE_BB = 
    STARTING_WHITE_PAWNS | STARTING_WHITE_ROOKS | STARTING_WHITE_KNIGHTS | STARTING_WHITE_BISHOPS | STARTING_WHITE_QUEENS | STARTING_WHITE_KINGS;

constexpr u64 STARTING_BLACK_BB = 
    STARTING_BLACK_PAWNS | STARTING_BLACK_ROOKS | STARTING_BLACK_KNIGHTS | STARTING_BLACK_BISHOPS | STARTING_BLACK_QUEENS | STARTING_BLACK_KINGS;

constexpr u64 STARTING_COMBINED_BB = STARTING_WHITE_BB | STARTING_BLACK_BB;

inline u64 STARTING_PAWN_MASK(Color c) noexcept {
  constexpr std::array<u64, 2> STARTING_PAWN_RANK_MASKS {
    STARTING_BLACK_PAWNS,
    STARTING_WHITE_PAWNS,
  };
  return STARTING_PAWN_RANK_MASKS[c == Color::WHITE];
}

inline u64 PAWN_PROMOTION_RANK_MASK(Color c) noexcept {
  constexpr std::array<u64, 2> PROMOTION_RANKS {
    0xFFULL,
    0xFFULL << (7 * FILES),
  };
  return PROMOTION_RANKS[c == Color::WHITE];
}

constexpr u64 DEFAULT_SEED = 0xdecafc0ffeecafeULL;
