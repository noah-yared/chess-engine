#pragma once

#include <algorithm>
#include <array>
#include <iostream>

#include "constants.h"
#include "directions.h"
#include "platform.h"


// Helper functions for attack calculations
template<int NumAttackVectors>
constexpr u64 nonSlidingAttacksBitmap(int pieceIndex, const std::array<std::pair<int, int>, NumAttackVectors>& attackVectors) {
    u64 attackBitmap {0ULL};
    int rowIndex {pieceIndex / 8};
    int colIndex {pieceIndex % 8};

    constexpr auto inBoard = [](int x, int y) -> bool { return (x < 8 && x >= 0 && y < 8 && y >= 0); };
    for (const auto [dr, dc] : attackVectors)
        if (inBoard(rowIndex + dr, colIndex + dc))
            attackBitmap |= 1ULL << (pieceIndex + dr * 8 + dc);

    return attackBitmap;
}

constexpr u64 getKingAttackBitmap(int pieceIndex) {
    constexpr std::array<std::pair<int, int>, 8> kingAttacks = {{{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}}};
    return nonSlidingAttacksBitmap<8>(pieceIndex, kingAttacks);
}

constexpr u64 getKnightAttackBitmap(int pieceIndex) {
    constexpr std::array<std::pair<int, int>, 8> knightAttacks = {{{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}}};
    return nonSlidingAttacksBitmap<8>(pieceIndex, knightAttacks);
}

constexpr u64 getAttackBitmapAlongDirection(int pieceIndex, Direction attackDirection) {
    u64 attackBitmap {0ULL};
    int sfamt = Directions::sfamt(attackDirection), numSqsInLane = Directions::numSqsInLane(attackDirection, pieceIndex);
    for (int bit = pieceIndex + sfamt; numSqsInLane--; bit += sfamt) 
        attackBitmap |= 1ULL << bit;
    return attackBitmap;
}

// Main attack bitmap generation functions
inline constexpr std::array<u64, SQUARES> compileKnightAttacks() {
    std::array<u64, SQUARES> knightAttacks{};
    for (int i = 0; i < SQUARES; i++) 
        knightAttacks[i] = getKnightAttackBitmap(i);
    return knightAttacks;
}

inline constexpr std::array<u64, SQUARES> compileKingAttacks() {
    std::array<u64, SQUARES> kingAttacks{};
    for (int i = 0; i < SQUARES; i++)
        kingAttacks[i] = getKingAttackBitmap(i);
    return kingAttacks;
}

inline constexpr std::array<std::array<u64, NUM_DIRECTIONS>, SQUARES> compileSlidingAttacks() {
    std::array<std::array<u64, NUM_DIRECTIONS>, SQUARES> slidingAttacks{};
    for (int i = 0; i < SQUARES; i++)
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
            slidingAttacks[i][dir] = getAttackBitmapAlongDirection(i, Direction(dir));
    return slidingAttacks;
}

// namespace OldAttacks {
// inline constexpr auto kingAttackBitmaps = compileKingAttacks();
// inline constexpr auto knightAttackBitmaps = compileKnightAttacks();
// inline constexpr auto slidingAttackBitmaps = compileSlidingAttacks();
// } // namespace Attacks

// Pre-computed attack bitmaps
class Attacks {
    static constexpr auto kingAttackBitmaps = compileKingAttacks();
    static constexpr auto knightAttackBitmaps = compileKnightAttacks();
    static constexpr auto slidingAttackBitmaps = compileSlidingAttacks();
public: 
    static constexpr auto getKingAttackBitmap(int pieceIndex) { return kingAttackBitmaps[pieceIndex]; }
    static constexpr auto getKnightAttackBitmap(int pieceIndex) { return knightAttackBitmaps[pieceIndex]; }
    static constexpr auto getSlidingAttackBitmap(int pieceIndex, Direction direction) {
        return slidingAttackBitmaps[pieceIndex][static_cast<int>(direction)];
    }
};
