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
  [[nodiscard]] static constexpr u64 combine(InputIt first, InputIt last) noexcept {
    return (first == last) ? 0ULL : std::accumulate(first, last, 0ULL, [](u64 acc, u64 bb) { return acc | bb; });
  }

  template <typename InputIt>
  requires std::input_iterator<InputIt> && std::is_same_v<std::iter_value_t<InputIt>, u64>
  [[nodiscard]] static constexpr PieceType findPiece(InputIt first, InputIt last, u64 mask) noexcept {
    auto found = std::find_if(first, last, [mask](u64 bb) -> bool { return bb & mask; });
    return found != last ? PieceType(std::distance(first, found) % NUM_PIECE_TYPES) : PieceType::NONE;
  }

  [[nodiscard]] constexpr int wKing() const noexcept { return BitUtils::ctz(bb(PieceType::KING, Color::WHITE)); }
  [[nodiscard]] constexpr int bKing() const noexcept { return BitUtils::ctz(bb(PieceType::KING, Color::BLACK)); }

  [[nodiscard]] static inline Color indexToColor(int index) {
    return Color(index >= NUM_PIECE_TYPES);
  }

 public:
  Bitboards() noexcept: bbs_(STARTING_BBS), whiteBB_(STARTING_WHITE_BB), blackBB_(STARTING_BLACK_BB) {};
  explicit Bitboards(const std::string& sBoard, FromAsciiBoard) : bbs_{}, whiteBB_{}, blackBB_{} {
    std::string pieceString = "prnbqkPRNBQK";
    int bit = 63;
    for (char c : sBoard) {
      if (iswspace(c))
        continue;
      if (auto pos = pieceString.find(c); pos != std::string::npos)
        togglePieceSquare(static_cast<int>(pos), bit);
      --bit;
    }
    if (bit != -1)
      throw std::runtime_error("Invalid ascii board format passed in! Make sure to specify all squares on the board!");
  }
  explicit Bitboards(const std::string& fen, FromFEN) : bbs_{}, whiteBB_{}, blackBB_{} {
    std::string pieceString = "prnbqkPRNBQK";
    std::string fenPieces = fen.substr(0, fen.find_first_of(' '));
    int bit = 63;
    for (auto c : fenPieces) {
      if (c == '/') continue;
      if (isdigit(c)) bit -= (c - '0');
      else togglePieceSquare(static_cast<int>(pieceString.find(c)), bit--);
    }
  };
  explicit Bitboards(const std::array<u64, NUM_BITBOARDS>& bbs) : bbs_{bbs} {
    whiteBB_ = combine(wStart(), wEnd());
    blackBB_ = combine(bStart(), bEnd());
  }
  explicit Bitboards(u64* const bbs) {
    assert(bbs != nullptr);
    std::copy(bbs, bbs + NUM_BITBOARDS, bbs_.begin());
    whiteBB_ = combine(wStart(), wEnd());
    blackBB_ = combine(bStart(), bEnd());
  };

  static Color keyToColor(int key) {
    return Color(key >= NUM_PIECE_TYPES);
  }

  static int pieceToKey(PieceType t, Color c) {
    return NUM_PIECE_TYPES * static_cast<int>(c) + static_cast<int>(t);
  }

  void togglePieceSquare(PieceType t, Color c, int square) noexcept {
    togglePieceSquare(pieceToKey(t, c), square);
  }

  void togglePieceSquare(int key, int square) noexcept {
    bbs_[key] ^= 1ULL << square;
    if (keyToColor(key) == Color::WHITE) 
      whiteBB_ ^= 1ULL << square;
    else
      blackBB_ ^= 1ULL << square;
  }

  // Iterators for all bitboards
  using iterator = std::array<u64, NUM_BITBOARDS>::const_iterator;
  [[nodiscard]] constexpr iterator begin() const noexcept { return bbs_.begin(); }
  [[nodiscard]] constexpr iterator end() const noexcept { return bbs_.end(); }

  // Iterators for black bitboards
  [[nodiscard]] constexpr iterator bStart() const noexcept { return bbs_.begin(); }
  [[nodiscard]] constexpr iterator bEnd() const noexcept { return std::next(bbs_.begin(), NUM_PIECE_TYPES); }

  // Iterators for white bitboards
  [[nodiscard]] constexpr iterator wStart() const noexcept { return bEnd(); }
  [[nodiscard]] constexpr iterator wEnd() const noexcept { return end(); }

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
      return findPiece(
        color == Color::WHITE ? wStart() : bStart(),
        color == Color::WHITE ? wEnd()   : bEnd(),
        pieceMask
      );
    return PieceType::NONE;
  }

  [[nodiscard]] constexpr bool operator==(const Bitboards& other) const noexcept { return bbs_ == other.bbs_; }
  [[nodiscard]] constexpr bool operator!=(const Bitboards& other) const noexcept { return bbs_ != other.bbs_; }

  [[nodiscard]] bool isConsistent() const noexcept {
    std::string pieceString = "prnbqkPRNBQK";
    if (whiteBB_ & blackBB_) return false;
    if (combine(wStart(), wEnd()) != whiteBB_) return false;
    if (combine(bStart(), bEnd()) != blackBB_) return false;
    for (int i = 0; i < NUM_BITBOARDS; ++i) {
      for (int j = i + 1; j < NUM_BITBOARDS; ++j) {
        if (bbs_[i] & bbs_[j]) {
          std::cout << pieceString[i] << "'s bitboard and " << pieceString[j] << "'s bitboard overlap!" << std::endl;
          return false;
        }
      }
    }
    return true;
  }

  [[nodiscard]] std::string toString() const {
    std::string pieceString = "prnbqkPRNBQK";
    std::stringstream ss;
    for (int r = 7; r >= 0; --r) {
      ss << "    "; // indent each rank
      for (int c = 7; c >= 0; --c) {
        int sq = r * FILES + c;
        if (auto pType = getPieceType(sq); pType != PieceType::NONE) {
          if (pType > PieceType::KING)
            throw std::runtime_error("Invalid piece type");
          bool isWhite = whiteBB() & (1ULL << sq);
          ss << pieceString[pieceToKey(pType, isWhite ? Color::WHITE : Color::BLACK)];
        } else {
          ss << '.';
        }
      }
      ss << '\n';
    }
    return ss.str();
  }

};

inline std::ostream& operator<<(std::ostream& os, const Bitboards& bitboards) noexcept {
  const auto pieceString = "prnbqkPRNBQK";
  os   << "Bitboards(\nbbs_=" 
        << "[\n";
  for (int i = 0; i < NUM_BITBOARDS; ++i) {
    os << "       " << pieceString[i] << ": " << std::bitset<64>(bitboards.bbs_[i]) << ",\n";
  }
  os   << "]\n"
        << "whiteBB_: " << std::bitset<64>(bitboards.whiteBB_) << "\n"
        << "blackBB_: " << std::bitset<64>(bitboards.blackBB_) << "\n)";
  return os;
};
