#include "attacks.h"

#include <array>
#include <iostream>
#include <algorithm>

#include "directions.h"
#include "utils.h"

typedef unsigned long long ull;

inline ull getKingAttackBitmap(int pieceIndex) {
  ull attackBitmap = 0ULL;
  int rowIndex = pieceIndex / 8;
  int colIndex = pieceIndex % 8;

  std::pair<int, int> kingAttacks[] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0},
                                       {1, 0},   {-1, 1}, {0, 1},  {1, 1}};

  for (auto move : kingAttacks) {
    if (inBoard(rowIndex + move.first, colIndex + move.second))
      attackBitmap |= 1ULL << (pieceIndex + move.first * 8 + move.second);
  }
  return attackBitmap;
}

inline ull getKnightAttackBitmap(int pieceIndex) {
  ull attackBitmap = 0ULL;
  int rowIndex = pieceIndex / 8;
  int colIndex = pieceIndex % 8;

  std::pair<int, int> knightAttacks[] = {{-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
                                         {1, -2},  {1, 2},  {2, -1},  {2, 1}};

  for (auto move : knightAttacks) {
    if (inBoard(rowIndex + move.first, colIndex + move.second)) 
      attackBitmap |= 1ULL << (pieceIndex + move.first * 8 + move.second);
  }
  return attackBitmap;
}

ull getAttackBitmapAlongDirection(const int pieceIndex,
                                  int attackDirection) {
  int rowIndex = 7 - pieceIndex / 8, colIndex = 7 - pieceIndex % 8;
  ull attackBitmap = 0ULL;
  int sfamt, sqs, bit;

  switch (attackDirection) {
    case EAST:
      sfamt = -1;
      sqs = 7 - colIndex;
      break;
    case WEST:
      sfamt = 1;
      sqs = colIndex;
      break;
    case NORTH:
      sfamt = 8;
      sqs = rowIndex;
      break;
    case SOUTH:
      sfamt = -8;
      sqs = 7 - rowIndex;
      break;
    case NORTHEAST:
      sfamt = 7;
      sqs = std::min(rowIndex, 7-colIndex);
      break;
    case SOUTHEAST:
      sfamt = -9;
      sqs = std::min(7-rowIndex, 7-colIndex);
      break;
    case NORTHWEST:
      sfamt = 9;
      sqs = std::min(rowIndex, colIndex);
      break;
    case SOUTHWEST: 
      sfamt = -7;
      sqs = std::min(7-rowIndex, colIndex);
      break;
    default: 
      std::cerr << attackDirection << " is invalid." << '\n';
      return ~0ULL;
  }

  bit = pieceIndex + sfamt;
  while (sqs--) {
    attackBitmap |= 1ULL << bit;
    bit += sfamt;
  }
  return attackBitmap;
}

std::array<ull, 64> compileKnightAttacks() {
  std::array<ull, 64> knightAttacks;
  for (int i = 0; i < 64; i++) knightAttacks[i] = getKnightAttackBitmap(i);
  return knightAttacks;
}

std::array<ull, 64> compileKingAttacks() {
  std::array<ull, 64> kingAttacks;
  for (int i = 0; i < 64; i++) {
    kingAttacks[i] = getKingAttackBitmap(i);
  }
  return kingAttacks;
}

std::array<std::array<ull, 8>, 64> compileSlidingAttacks() {
  std::array<std::array<ull, 8>, 64> slidingAttacks;
  for (int i = 0; i < 64; i++) {
    for (int dir = 0; dir < 8; dir++) {
      slidingAttacks[i][dir] = getAttackBitmapAlongDirection(i, dir);
    }
  }
  return slidingAttacks;
}
