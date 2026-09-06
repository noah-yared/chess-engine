#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <string>

#include "board/pieces.h"
#include "board/squares.h"
#include "util/platform.h"

// different types of moves
enum class MoveType
{
    Normal,
    Enpassant,
    Promotion,
    Castle,
    DoublePawnPush
};

// can capture in traditional sense (excludes enpassant, end square must contain opposing piece)
template <MoveType mType>
concept CanCapture = (mType == MoveType::Normal || mType == MoveType::Promotion);

// ROOK, KNIGHT, BISHOP, QUEEN are contiguous in PieceType, so a promotion
// target packs into two bits as `piece - ROOK`.
constexpr int PROMOTION_PIECE_OFFSET = static_cast<int>(PieceType::ROOK);

[[nodiscard]] constexpr char promotionPieceToChar(PieceType piece) noexcept
{
    switch (piece)
    {
    case PieceType::ROOK:
        return 'r';
    case PieceType::KNIGHT:
        return 'n';
    case PieceType::BISHOP:
        return 'b';
    default:
        return 'q';
    }
}

[[nodiscard]] constexpr PieceType charToPromotionPiece(char c) noexcept
{
    switch (c)
    {
    case 'r':
    case 'R':
        return PieceType::ROOK;
    case 'n':
    case 'N':
        return PieceType::KNIGHT;
    case 'b':
    case 'B':
        return PieceType::BISHOP;
    default:
        return PieceType::QUEEN;
    }
}

/*
 * A move packed into 32 bits:
 *
 *   bits  0-5   start square
 *   bits  6-11  end square
 *   bits 12-14  move type
 *   bits 15-16  promotion piece, as `piece - ROOK` (zero for non-promotions)
 *   bits 17-19  moved piece
 *   bits 20-22  captured piece (PieceType::NONE when the move is not a capture)
 *   bit  23     side to move
 */
class Move
{
  public:
    Move() noexcept = default;

    Move(MoveType type, unsigned int start, unsigned int end, Color side, PieceType moved,
         std::optional<PieceType> captured = std::nullopt,
         PieceType promotionPiece = PieceType::QUEEN) noexcept
        : data_{pack(type, start, end, side, moved, captured.value_or(PieceType::NONE),
                     promotionPiece)}
    {
    }

    [[nodiscard]] int start() const noexcept { return field(START_OFFSET, SQUARE_WIDTH); }
    [[nodiscard]] int end() const noexcept { return field(END_OFFSET, SQUARE_WIDTH); }
    [[nodiscard]] MoveType type() const noexcept { return MoveType(field(TYPE_OFFSET, TYPE_WIDTH)); }
    [[nodiscard]] Color side() const noexcept { return Color(field(SIDE_OFFSET, SIDE_WIDTH)); }
    [[nodiscard]] Color oppSide() const noexcept { return opposite(side()); }
    [[nodiscard]] PieceType moved() const noexcept
    {
        return PieceType(field(MOVED_OFFSET, PIECE_WIDTH));
    }
    [[nodiscard]] PieceType captured() const noexcept
    {
        return PieceType(field(CAPTURED_OFFSET, PIECE_WIDTH));
    }
    [[nodiscard]] bool isCapture() const noexcept { return captured() != PieceType::NONE; }

    // Only meaningful when type() == MoveType::Promotion.
    [[nodiscard]] PieceType promotionPiece() const noexcept
    {
        return PieceType(promotionBits() + PROMOTION_PIECE_OFFSET);
    }

    // Square of the pawn removed by an en passant capture, which is neither the
    // start nor the end square of the capturing pawn.
    [[nodiscard]] int enpassantCaptureSquare() const noexcept
    {
        return (start() & 56) + (end() & 7);
    }

    // Square a double pawn push skips over, which becomes the en passant target.
    [[nodiscard]] int enpassantTargetSquare() const noexcept { return (start() + end()) / 2; }

    // branchless optimization
    [[nodiscard]] int castledRookStart() const noexcept
    {
        int queenSide = start() < end(), blackCastle = side() == Color::BLACK;
        return 7 * queenSide + 56 * blackCastle;
    }
    [[nodiscard]] int castledRookEnd() const noexcept { return (start() + end()) / 2; }

    [[nodiscard]] std::string uci() const noexcept
    {
        std::string move = indexToAlgebraicNotation(start()) + indexToAlgebraicNotation(end());
        if (type() == MoveType::Promotion)
            move += promotionPieceToChar(promotionPiece());
        return move;
    }

    [[nodiscard]] u32 bits() const noexcept { return data_; }

    // operator overloads
    bool operator==(const Move other) const noexcept { return data_ == other.data_; }
    bool operator!=(const Move other) const noexcept { return data_ != other.data_; }

    void print() const noexcept
    {
        std::cout << uci();
        switch (type())
        {
        case MoveType::Enpassant:
            std::cout << " (ep)";
            break;
        case MoveType::DoublePawnPush:
            std::cout << " (dp)";
            break;
        case MoveType::Castle:
            std::cout << " ("
                      << (side() == Color::WHITE ? (start() < end() ? "O-O-O" : "O-O")
                                                 : (start() < end() ? "o-o-o" : "o-o"))
                      << ')';
            break;
        default:
            break;
        };
        std::cout << '\n';
    }

  private:
    static constexpr int SQUARE_WIDTH = 6;
    static constexpr int TYPE_WIDTH = 3;
    static constexpr int PROMO_WIDTH = 2;
    static constexpr int PIECE_WIDTH = 3;
    static constexpr int SIDE_WIDTH = 1;

    static constexpr int START_OFFSET = 0;
    static constexpr int END_OFFSET = START_OFFSET + SQUARE_WIDTH;
    static constexpr int TYPE_OFFSET = END_OFFSET + SQUARE_WIDTH;
    static constexpr int PROMO_OFFSET = TYPE_OFFSET + TYPE_WIDTH;
    static constexpr int MOVED_OFFSET = PROMO_OFFSET + PROMO_WIDTH;
    static constexpr int CAPTURED_OFFSET = MOVED_OFFSET + PIECE_WIDTH;
    static constexpr int SIDE_OFFSET = CAPTURED_OFFSET + PIECE_WIDTH;

    static_assert(SIDE_OFFSET + SIDE_WIDTH <= 32, "Move fields must fit in 32 bits");

    [[nodiscard]] int field(int offset, int width) const noexcept
    {
        return (data_ >> offset) & ((1u << width) - 1);
    }

    [[nodiscard]] int promotionBits() const noexcept { return field(PROMO_OFFSET, PROMO_WIDTH); }

    static constexpr u32 pack(MoveType type, unsigned int start, unsigned int end, Color side,
                              PieceType moved, PieceType captured,
                              PieceType promotionPiece) noexcept
    {
        // Canonicalize the promotion bits so that equality and orderingKey()
        // stay exact for the move types that never promote.
        const u32 promoBits =
            type == MoveType::Promotion
                ? static_cast<u32>(static_cast<int>(promotionPiece) - PROMOTION_PIECE_OFFSET)
                : 0u;
        return (static_cast<u32>(start) << START_OFFSET) |
               (static_cast<u32>(end) << END_OFFSET) |
               (static_cast<u32>(type) << TYPE_OFFSET) | (promoBits << PROMO_OFFSET) |
               (static_cast<u32>(moved) << MOVED_OFFSET) |
               (static_cast<u32>(captured) << CAPTURED_OFFSET) |
               (static_cast<u32>(side) << SIDE_OFFSET);
    }

    u32 data_ = 0;
};

static_assert(sizeof(Move) == sizeof(u32), "Move must stay packed into 32 bits");

// hash function for move
namespace std
{
template <>
struct hash<Move>
{
    size_t operator()(const Move move) const noexcept
    {
        return std::hash<u32>()(move.bits());
    }
};
} // namespace std
