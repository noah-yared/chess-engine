#pragma once

#include <iostream>
#include <cctype>
#include <concepts>
#include <optional>
#include <string>
#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "bitboards.h"
#include "board_utils.h"
#include "pieces.h"

// different types of moves
enum class MoveType {
  Normal, Enpassant, Promotion, Castle, DoublePawnPush
};

// can capture in traditional sense (excludes enpassant, end square must contain opposing piece)
template<MoveType mType>
concept CanCapture = (mType == MoveType::Normal || mType == MoveType::Promotion);

template<MoveType mType>
struct MoveBase {
public:
  static constexpr auto type = mType;

  // default constructor, copy constructor, and copy assignment operator
  MoveBase() noexcept = default; 
  MoveBase(const MoveBase&) noexcept = default;
  MoveBase& operator=(const MoveBase&) noexcept = default;

  MoveBase(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt) noexcept
    : start_(start), 
      end_(end),
      side_(static_cast<unsigned int>(side)),
      moved_(static_cast<unsigned int>(moved)),
      isCapture_(captured.has_value()),
      captured_(static_cast<unsigned int>(captured.value_or(PieceType::NONE))) {};

private:
  unsigned int start_        : 6,
               end_          : 6,
               side_         : 1,
               moved_        : 3,
               isCapture_    : 1,
               captured_     : 3;

public:
  [[nodiscard]] int start() const noexcept { return start_; }
  [[nodiscard]] int end() const noexcept { return end_; }
  [[nodiscard]] Color side() const noexcept { return Color(side_); }
  [[nodiscard]] Color oppSide() const noexcept { return opposite(side()); }
  [[nodiscard]] PieceType moved() const noexcept { return PieceType(moved_); }
  [[nodiscard]] PieceType captured() const noexcept { return PieceType(captured_); }
  
  // bitboard key helpers
  [[nodiscard]] int movedKey() const noexcept { return Bitboards::pieceToKey(moved(), side()); }
  [[nodiscard]] std::optional<int> capturedKey() const noexcept { return isCapture_ ? std::optional<int>(Bitboards::pieceToKey(captured(), oppSide())) : std::nullopt; }
  
  [[nodiscard]] std::string uci() const noexcept {
    std::stringstream ss;
    ss << indexToAlgebraicNotation(start_) << indexToAlgebraicNotation(end_);
    if (mType == MoveType::Promotion) ss << 'q'; // always promote to queen
    return ss.str();
  }

  // operator overloads
  template<MoveType OtherMoveType>
  bool operator==(const MoveBase<OtherMoveType> other) const noexcept {
    return start() == other.start() && end() == other.end()
        && moved() == other.moved() && captured() == other.captured()
        && mType == OtherMoveType;
  }
  template<MoveType OtherMoveType>
  bool operator!=(const MoveBase<OtherMoveType> other) const noexcept {
    return !operator==(other);
  }

  void print() const noexcept {
    std::cout << uci();
    switch (type) {
      case MoveType::Enpassant: std::cout << " (ep)"; break;
      case MoveType::DoublePawnPush: std::cout << " (dp)"; break;
      case MoveType::Castle:
        std::cout << " (" << (side() == Color::WHITE
            ? (start() < end() ? "O-O-O" : "O-O")
            : (start() < end() ? "o-o-o" : "o-o")) << ')';
        break;
      default: break;
    };
    std::cout << '\n';
  }
};

template<MoveType mType>
struct Move : public MoveBase<mType> {
  // default constructor, copy constructor, and copy assignment operator
  Move() noexcept = default;
  Move(const Move&) noexcept = default;
  Move& operator=(const Move&) noexcept = default;

  Move(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt) noexcept
    : MoveBase<mType>(start, end, side, moved, captured) {};
};

template<>
struct Move<MoveType::Promotion> : public MoveBase<MoveType::Promotion> {
  // default constructor, copy constructor, and copy assignment operator
  Move() noexcept = default;
  Move(const Move&) noexcept = default;
  Move& operator=(const Move&) noexcept = default;

  Move(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt) noexcept
    : MoveBase<MoveType::Promotion>(start, end, side, moved, captured) {};

  // promote to queen, save time searching game tree for different
  // promotion options when queen promotion is optimal in over 95% of cases
  [[nodiscard]] PieceType promotionPiece() const noexcept { return PieceType::QUEEN; }
  [[nodiscard]] int promotionKey() const noexcept { return Bitboards::pieceToKey(PieceType::QUEEN, side()); }
};

template<>
struct Move<MoveType::Castle> : public MoveBase<MoveType::Castle> {
  // default constructor, copy constructor, and copy assignment operator
  Move() noexcept = default;
  Move(const Move&) noexcept = default;
  Move& operator=(const Move&) noexcept = default;

  Move(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt) noexcept
    : MoveBase<MoveType::Castle>(start, end, side, moved, captured) {};

  // branchless optimization
  [[nodiscard]] int castledRookStart() const noexcept {
    int queenSide = start() < end(), blackCastle = side() == Color::BLACK;
    return 7 * queenSide + 56 * blackCastle;
  }
  [[nodiscard]] int castledRookEnd() const noexcept { return (start() + end()) / 2; }
  [[nodiscard]] int castledRookKey() const noexcept { return Bitboards::pieceToKey(PieceType::ROOK, side()); }
};

template<>
struct Move<MoveType::Enpassant> : public MoveBase<MoveType::Enpassant> {
  // default constructor, copy constructor, and copy assignment operator
  Move() noexcept = default;
  Move(const Move&) noexcept = default;
  Move& operator=(const Move&) noexcept = default;

  Move(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt) noexcept
    : MoveBase<MoveType::Enpassant>(start, end, side, moved, captured) {};

  [[nodiscard]] int enpassantSquare() const noexcept { return (start() & 56) + (end() & 7); }
  [[nodiscard]] int enpassantKey() const noexcept { return Bitboards::pieceToKey(PieceType::PAWN, oppSide()); }
};

template<>
struct Move<MoveType::DoublePawnPush> : public MoveBase<MoveType::DoublePawnPush> {
  // default constructor, copy constructor, and copy assignment operator
  Move() noexcept = default;
  Move(const Move&) noexcept = default;
  Move& operator=(const Move&) noexcept = default;

  Move(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt) noexcept
    : MoveBase<MoveType::DoublePawnPush>(start, end, side, moved, captured) {};

  [[nodiscard]] int enpassantSquare() const noexcept { return (start() + end()) / 2; }
  [[nodiscard]] int enpassantKey() const noexcept { return Bitboards::pieceToKey(PieceType::PAWN, side()); }
};

// hash function for move
namespace std {
  template<MoveType mType>
  struct hash<Move<mType>> {
    size_t operator()(const Move<mType> move) const noexcept {
      return move.start() ^ move.end() ^ std::hash<PieceType>()(move.moved()) << 2 ^ std::hash<PieceType>()(move.captured());
    }
  };
}
