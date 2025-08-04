#pragma once

#include "bitboards.h"
#include "pieces.h"

/*
 * store following info for piece square updates:
 * @key: piece bitboard index
 * @square: bit index of piece bitboard 
 */
struct Delta {
  int key, square;

  static Delta Place(int key, int square) noexcept { return { key, square }; }
  static Delta Remove(int key, int square) noexcept { return { key, square }; }
};
