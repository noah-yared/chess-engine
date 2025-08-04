#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>

#include "bit_utils.h"
#include "constants.h"
#include "directions.h"
#include "pieces.h"
#include "platform.h"

struct FromFEN {};
struct FromAsciiBoard {};

class Bitboards {
  std::array<u64, NUM_BITBOARDS> bbs_; // order: p, r, n, b, q, k, P, R, N, B, Q, K
  u64 whiteBB_, blackBB_; // combined = white | black

  friend std::ostream& operator<<(std::ostream&, const Bitboards&) noexcept;

  template <typename InputIt>
  requires std::input_iterator<InputIt> && std::is_same_v<std::iter_value_t<InputIt>, u64>
  [[nodiscard]] static u64 combine(InputIt first, InputIt last) noexcept {
    return (first == last) ? 0ULL : std::accumulate(first, last, 0ULL, [](u64 acc, u64 bb) noexcept { return acc | bb; });
  }

  template <typename InputIt>
  requires std::input_iterator<InputIt> && std::is_same_v<std::iter_value_t<InputIt>, u64>
  [[nodiscard]] static PieceType findPiece(InputIt first, InputIt last, u64 mask) noexcept {
    auto found = std::find_if(first, last, [mask](u64 bb) noexcept -> bool { return bb & mask; });
    return found != last ? PieceType(std::distance(first, found) % NUM_PIECE_TYPES) : PieceType::NONE;
  }

  [[nodiscard]] int wKing() const noexcept { return BitUtils::ctz(bb(PieceType::KING, Color::WHITE)); }
  [[nodiscard]] int bKing() const noexcept { return BitUtils::ctz(bb(PieceType::KING, Color::BLACK)); }

  [[nodiscard]] static inline Color indexToColor(int index) noexcept {
    return Color(index >= NUM_PIECE_TYPES);
  }

 public:
  // Simple constructors stay in header
  Bitboards() noexcept: bbs_(STARTING_BBS), whiteBB_(STARTING_WHITE_BB), blackBB_(STARTING_BLACK_BB) {};
  explicit Bitboards(const std::array<u64, NUM_BITBOARDS>& bbs) : bbs_{bbs} {
    whiteBB_ = combine(wStart(), wEnd());
    blackBB_ = combine(bStart(), bEnd());
  }
  explicit Bitboards(u64* const bbs) noexcept {
    assert(bbs != nullptr);
    std::copy(bbs, bbs + NUM_BITBOARDS, bbs_.begin());
    whiteBB_ = combine(wStart(), wEnd());
    blackBB_ = combine(bStart(), bEnd());
  };

  // Complex constructors moved to source file
  explicit Bitboards(const std::string& sBoard, FromAsciiBoard) noexcept;
  explicit Bitboards(const std::string& fen, FromFEN) noexcept;

  static Color keyToColor(int key) noexcept {
    return Color(key >= NUM_PIECE_TYPES);
  }

  static int pieceToKey(PieceType t, Color c) noexcept {
    return NUM_PIECE_TYPES * static_cast<int>(c) + static_cast<int>(t);
  }

  void togglePieceSquare(PieceType t, Color c, int square) noexcept {
    togglePieceSquare(pieceToKey(t, c), square);
  }

  void togglePieceSquare(int key, int square) noexcept {
    bbs_[key] ^= 1ULL << square;
    keyToColor(key) == Color::WHITE ? (whiteBB_ ^= 1ULL << square) : (blackBB_ ^= 1ULL << square);
  }

  // Iterators for all bitboards
  using iterator = std::array<u64, NUM_BITBOARDS>::const_iterator;
  [[nodiscard]] iterator begin() const noexcept { return bbs_.begin(); }
  [[nodiscard]] iterator end() const noexcept { return bbs_.end(); }

  // Iterators for black bitboards
  [[nodiscard]] iterator bStart() const noexcept { return bbs_.begin(); }
  [[nodiscard]] iterator bEnd() const noexcept { return std::next(bbs_.begin(), NUM_PIECE_TYPES); }

  // Iterators for white bitboards
  [[nodiscard]] iterator wStart() const noexcept { return bEnd(); }
  [[nodiscard]] iterator wEnd() const noexcept { return end(); }

  // King square retrieval
  [[nodiscard]] int king(Color c) const noexcept { return c == Color::WHITE ? wKing() : bKing(); }

  // Bitboards reading
  [[nodiscard]] u64 bb(PieceType t, Color c) const noexcept { return bbs_[pieceToKey(t, c)]; }
  [[nodiscard]] u64 bb(int bbKey) const noexcept { return bbs_[bbKey]; }

  [[nodiscard]] u64 whiteBB() const noexcept { return whiteBB_; }
  [[nodiscard]] u64 blackBB() const noexcept { return blackBB_; }
  [[nodiscard]] u64 combinedBB() const noexcept { return whiteBB_ | blackBB_; }

  [[nodiscard]] u64 allyBB(Color color) const noexcept { return color == Color::WHITE ? whiteBB_ : blackBB_; }
  [[nodiscard]] u64 opposingBB(Color color) const noexcept { return color == Color::WHITE ? blackBB_ : whiteBB_; }

  // Piece type retrieval
  [[nodiscard]] PieceType getPieceType(int square) const noexcept {
    if (u64 pieceMask = 1ULL << square; combinedBB() & pieceMask)
      return findPiece(begin(), end(), pieceMask);
    return PieceType::NONE;
  }

  [[nodiscard]] PieceType getPieceType(int square, Color color) const noexcept {
    if (u64 pieceMask = 1ULL << square; allyBB(color) & pieceMask)
      return color == Color::WHITE
        ? findPiece(wStart(), wEnd(), pieceMask)
        : findPiece(bStart(), bEnd(), pieceMask);
    return PieceType::NONE;
  }

  // Piece must exist on the square
  [[nodiscard]] Color getPieceColor(int square) const noexcept {
    return whiteBB_ & (1ULL << square) ? Color::WHITE : Color::BLACK;
  }

  [[nodiscard]] bool operator==(const Bitboards& other) const noexcept { return bbs_ == other.bbs_; }
  [[nodiscard]] bool operator!=(const Bitboards& other) const noexcept { return bbs_ != other.bbs_; }

  // Complex methods moved to source file
  [[nodiscard]] bool isConsistent() const noexcept;
  [[nodiscard]] std::string parsePiecePlacement() const noexcept;
  [[nodiscard]] std::string toString() const noexcept;
};
