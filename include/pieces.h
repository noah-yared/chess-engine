#pragma once

#include <concepts>

enum class PieceType
{
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    NONE
};

enum class Color
{
    BLACK,
    WHITE
};

// inline Color opposite(Color c) { return c == Color::WHITE ? Color::BLACK : Color::WHITE; }
template<Color c>
inline consteval Color opposite() { return c == Color::WHITE ? Color::BLACK : Color::WHITE; }
inline Color opposite(Color c) noexcept { return Color(c != Color::WHITE); }
inline PieceType& operator++(PieceType& t) noexcept
{
    return t = PieceType(static_cast<int>(t) + 1);
}

// useful concepts
template <PieceType pType>
concept Sliding =
    (pType == PieceType::QUEEN || pType == PieceType::ROOK || pType == PieceType::BISHOP);

template <PieceType pType>
concept NonSliding =
    (pType == PieceType::PAWN || pType == PieceType::KNIGHT || pType == PieceType::KING);
