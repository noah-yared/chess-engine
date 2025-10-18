#pragma once

#include "constants.h"
#include "directions.h"
#include "pieces.h"

// color traits
template <Color c>
struct ColorTraits
{
    // Movement directions
    static constexpr Direction forward = c == Color::WHITE ? Direction::N : Direction::S;
    static constexpr Direction backward = c == Color::WHITE ? Direction::S : Direction::N;

    // Pawn attack directions
    static constexpr Direction leftPawnAttack = c == Color::WHITE ? Direction::NW : Direction::SW;
    static constexpr Direction rightPawnAttack = c == Color::WHITE ? Direction::NE : Direction::SE;

    // Ranks
    static constexpr int pawnRank = c == Color::WHITE ? 2 : 7;
    static constexpr int promotionRank = c == Color::WHITE ? 8 : 1;
    static constexpr int backRank = c == Color::WHITE ? 1 : 8;
    static constexpr int rankDelta = c == Color::WHITE ? 1 : -1;
};

// Convenience aliases for common usage
template <Color c>
inline constexpr Direction forward = ColorTraits<c>::forward;
template <Color c>
inline constexpr Direction backward = ColorTraits<c>::backward;
template <Color c>
inline constexpr Direction leftPawnAttack = ColorTraits<c>::leftPawnAttack;
template <Color c>
inline constexpr Direction rightPawnAttack = ColorTraits<c>::rightPawnAttack;
template <Color c>
inline constexpr int pawnRank = ColorTraits<c>::pawnRank;
template <Color c>
inline constexpr int promotionRank = ColorTraits<c>::promotionRank;
template <Color c>
inline constexpr int backRank = ColorTraits<c>::backRank;
template <Color c>
inline constexpr int rankDelta = ColorTraits<c>::rankDelta;

// piece traits
template <PieceType P, Color C>
struct PieceTraits
{
};
template <>
struct PieceTraits<PieceType::PAWN, Color::BLACK>
{
};
template <>
struct PieceTraits<PieceType::PAWN, Color::WHITE>
{
    static constexpr int bb_index = 6;
};
template <>
struct PieceTraits<PieceType::ROOK, Color::BLACK>
{
    static constexpr int bb_index = 1;
};
template <>
struct PieceTraits<PieceType::ROOK, Color::WHITE>
{
    static constexpr int bb_index = 7;
};
template <>
struct PieceTraits<PieceType::KNIGHT, Color::BLACK>
{
    static constexpr int bb_index = 2;
};
template <>
struct PieceTraits<PieceType::KNIGHT, Color::WHITE>
{
    static constexpr int bb_index = 8;
};
template <>
struct PieceTraits<PieceType::BISHOP, Color::BLACK>
{
    static constexpr int bb_index = 3;
};
template <>
struct PieceTraits<PieceType::BISHOP, Color::WHITE>
{
    static constexpr int bb_index = 9;
};
template <>
struct PieceTraits<PieceType::QUEEN, Color::BLACK>
{
    static constexpr int bb_index = 4;
};
template <>
struct PieceTraits<PieceType::QUEEN, Color::WHITE>
{
    static constexpr int bb_index = 10;
};
template <>
struct PieceTraits<PieceType::KING, Color::BLACK>
{
    static constexpr int bb_index = 5;
};
template <>
struct PieceTraits<PieceType::KING, Color::WHITE>
{
    static constexpr int bb_index = 11;
};

// castling traits
template <char CastleType>
struct CastlingTraits
{
};
template <>
struct CastlingTraits<'k'>
{
    static constexpr int bit_offset = PRIV_BIT_OFFSET('k'), destination = 57;
};
template <>
struct CastlingTraits<'q'>
{
    static constexpr int bit_offset = PRIV_BIT_OFFSET('q'), destination = 61;
};
template <>
struct CastlingTraits<'K'>
{
    static constexpr int bit_offset = PRIV_BIT_OFFSET('K'), destination = 1;
};
template <>
struct CastlingTraits<'Q'>
{
    static constexpr int bit_offset = PRIV_BIT_OFFSET('Q'), destination = 5;
};

template <int Offset>
struct OffsetToPriv
{
};
template <>
struct OffsetToPriv<0>
{
    static constexpr char priv = 'k';
};
template <>
struct OffsetToPriv<1>
{
    static constexpr char priv = 'q';
};
template <>
struct OffsetToPriv<2>
{
    static constexpr char priv = 'K';
};
template <>
struct OffsetToPriv<3>
{
    static constexpr char priv = 'Q';
};
