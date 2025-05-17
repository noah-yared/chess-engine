#ifndef ATTACKS_H
#define ATTACKS_H

#include <array>

typedef unsigned long long ull;

std::array<ull, 64> compileKingAttacks();
std::array<ull, 64> compileKnightAttacks();
std::array<std::array<ull, 8>, 64> compileSlidingAttacks();

// setup attack bitmaps
inline const auto kingAttackBitmaps = compileKingAttacks();
inline const auto knightAttackBitmaps = compileKnightAttacks();
inline const auto slidingAttackBitmaps = compileSlidingAttacks();

#endif