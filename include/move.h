#pragma once

#include <iostream>
#include <cctype>
#include <concepts>
#include <optional>
#include <string>
#include <sstream>
#include <unordered_map>

#include "bitboards.h"
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
  MoveBase() = default; 
  MoveBase(const MoveBase&) = default;
  MoveBase& operator=(const MoveBase&) = default;

  MoveBase(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt)
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
  [[nodiscard]] int start() const { return start_; }
  [[nodiscard]] int end() const { return end_; }
  [[nodiscard]] Color side() const { return Color(side_); }
  [[nodiscard]] Color oppSide() const { return opposite(side()); }
  [[nodiscard]] PieceType moved() const { return PieceType(moved_); }
  [[nodiscard]] PieceType captured() const { return PieceType(captured_); }
  
  // bitboard key helpers
  [[nodiscard]] int movedKey() const { return Bitboards::pieceToKey(moved(), side()); }
  [[nodiscard]] std::optional<int> capturedKey() const { return isCapture_ ? std::optional<int>(Bitboards::pieceToKey(captured(), oppSide())) : std::nullopt; }
  
  [[nodiscard]] std::string uci() const {
    std::stringstream ss;
    ss << getAlgebraicNotation(start_) << getAlgebraicNotation(end_);
    return ss.str();
  }

  [[nodiscard]] static std::string getAlgebraicNotation(int square) {
    std::stringstream ss;
    char file = static_cast<char>('a' + (7 - (square % RANKS)));
    char rank = static_cast<char>('1' + (square / FILES));
    ss << file << rank;
    return ss.str();
  }

  [[nodiscard]] static std::string getStringNotation(int square) {
    std::stringstream ss;
    int row = 8 - (square % 8);
    int col = 1 + (square / 8);
    ss << '(' << row << ", " << col << ')';
    return ss.str();
  }

  static inline const std::unordered_map<PieceType, char> pieceToChar = {
    {PieceType::PAWN,   'P'},
    {PieceType::KNIGHT, 'N'},
    {PieceType::BISHOP, 'B'},
    {PieceType::ROOK,   'R'},
    {PieceType::QUEEN,  'Q'},
    {PieceType::KING,   'K'},
  };

  void print() const {
    // helper lambda functions
    const auto getPieceChar = [this](PieceType piece) -> char {
      return (side() == Color::WHITE ? pieceToChar.at(piece) 
                                     : std::tolower(pieceToChar.at(piece)));
    };

    std::cout << getPieceChar(moved()) << ": " << getStringNotation(start_) << " to " << getStringNotation(end_);
    switch (mType) {
      // case MoveType::Capture: std::cout << ", x"; break; //<< getPieceChar(captured()); break;
      case MoveType::Enpassant: std::cout << ", e.p."; break;
      case MoveType::Promotion: std::cout << ", =Q"; break;
      case MoveType::DoublePawnPush: std::cout << ", P.P"; break;
      case MoveType::Castle: std::cout << ", " << (side() == Color::WHITE ? "O-O-O" : "O-O"); break;
      default: break; // normal/quiet (may include capture)
    };
  }
};

template<MoveType mType>
struct Move : public MoveBase<mType> {
  // default constructor, copy constructor, and copy assignment operator
  Move() = default;
  Move(const Move&) = default;
  Move& operator=(const Move&) = default;

  Move(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt)
    : MoveBase<mType>(start, end, side, moved, captured) {};
};

template<>
struct Move<MoveType::Promotion> : public MoveBase<MoveType::Promotion> {
  // default constructor, copy constructor, and copy assignment operator
  Move<MoveType::Promotion>() = default;
  Move<MoveType::Promotion>(const Move<MoveType::Promotion>&) = default;
  Move<MoveType::Promotion>& operator=(const Move<MoveType::Promotion>&) = default;

  Move<MoveType::Promotion>(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt)
    : MoveBase<MoveType::Promotion>(start, end, side, moved, captured) {};

  // promote to queen, save time searching game tree for different
  // promotion options when queen promotion is optimal in over 95% of cases
  [[nodiscard]] PieceType promotionPiece() const { return PieceType::QUEEN; }
  [[nodiscard]] int promotionKey() const { return Bitboards::pieceToKey(PieceType::QUEEN, side()); }
};

template<>
struct Move<MoveType::Castle> : public MoveBase<MoveType::Castle> {
  // default constructor, copy constructor, and copy assignment operator
  Move<MoveType::Castle>() = default;
  Move<MoveType::Castle>(const Move<MoveType::Castle>&) = default;
  Move<MoveType::Castle>& operator=(const Move<MoveType::Castle>&) = default;

  Move<MoveType::Castle>(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt)
    : MoveBase<MoveType::Castle>(start, end, side, moved, captured) {};

  // branchless optimization
  [[nodiscard]] int castledRookStart() const {
    int queenSide = start() < end(), blackCastle = side() == Color::BLACK;
    return 7 * queenSide + 56 * blackCastle;
  }
  [[nodiscard]] int castledRookEnd() const { return (start() + end()) / 2; }
  [[nodiscard]] int castledRookKey() const { return Bitboards::pieceToKey(PieceType::ROOK, side()); }
};

template<>
struct Move<MoveType::Enpassant> : public MoveBase<MoveType::Enpassant> {
  // default constructor, copy constructor, and copy assignment operator
  Move<MoveType::Enpassant>() = default;
  Move<MoveType::Enpassant>(const Move<MoveType::Enpassant>&) = default;
  Move<MoveType::Enpassant>& operator=(const Move<MoveType::Enpassant>&) = default;

  Move<MoveType::Enpassant>(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt)
    : MoveBase<MoveType::Enpassant>(start, end, side, moved, captured) {};

  [[nodiscard]] int enpassantSquare() const { return (start() & 56) + (end() & 7); }
  [[nodiscard]] int enpassantKey() const { return Bitboards::pieceToKey(PieceType::PAWN, oppSide()); }
};

template<>
struct Move<MoveType::DoublePawnPush> : public MoveBase<MoveType::DoublePawnPush> {
  // default constructor, copy constructor, and copy assignment operator
  Move<MoveType::DoublePawnPush>() = default;
  Move<MoveType::DoublePawnPush>(const Move<MoveType::DoublePawnPush>&) = default;
  Move<MoveType::DoublePawnPush>& operator=(const Move<MoveType::DoublePawnPush>&) = default;

  Move<MoveType::DoublePawnPush>(unsigned int start, unsigned int end, Color side, PieceType moved, std::optional<PieceType> captured=std::nullopt)
    : MoveBase<MoveType::DoublePawnPush>(start, end, side, moved, captured) {};

  [[nodiscard]] int enpassantSquare() const { return (start() + end()) / 2; }
  [[nodiscard]] int enpassantKey() const { return Bitboards::pieceToKey(PieceType::PAWN, side()); }
};
