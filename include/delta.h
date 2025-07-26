#pragma once

#include "bitboards.h"
#include "pieces.h"

/*
 * store following info for piece square updates:
 * @id: piece bitboard index
 * @square: bit index of piece bitboard 
 * @isRemoval: true indicates piece removed, false indicates placement
 */
struct Delta {
  enum Action { ADDED, REMOVED };
  int key, square;
  Action change;

  static Delta Place(int key, int square) {
    return {
      .key = key,
      .square = square,
      .change = ADDED,
    };
  }

  static Delta Remove(int key, int square) {
    return {
      .key = key,
      .square = square,
      .change = REMOVED,
    };
  }

};