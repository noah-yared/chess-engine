#pragma once

#include <string>
#include <sstream>

#include "constants.h"
#include "platform.h"

enum Square : unsigned int {
  H1, G1, F1, E1, D1, C1, B1, A1,
  H2, G2, F2, E2, D2, C2, B2, A2,
  H3, G3, F3, E3, D3, C3, B3, A3,
  H4, G4, F4, E4, D4, C4, B4, A4,
  H5, G5, F5, E5, D5, C5, B5, A5,
  H6, G6, F6, E6, D6, C6, B6, A6,
  H7, G7, F7, E7, D7, C7, B7, A7,
  H8, G8, F8, E8, D8, C8, B8, A8,
  NUM_SQUARES
};

inline void printBitboard(u64 bb) {
  for (int r = 7; r >= 0; --r) {
    for (int c = 7; c >= 0; --c)
      std::cout << ((bb & (1ULL << (r * FILES + c))) ? 'X' : '.');
    std::cout << '\n';
  }
}

inline int algebraicNotationToIndex(std::string algebraicNotation) {
  assert(algebraicNotation.size() == 2);
  return 7 - (algebraicNotation[0] - 'a') + ((algebraicNotation[1] - '1') << 3);
}

inline std::string indexToAlgebraicNotation(int index) {
  std::stringstream ss;
  char rank = '1' + (index / FILES);
  char file = 'a' + 7 - (index % RANKS);
  ss << file << rank;
  return ss.str();
}

inline bool isSquareOnLeftEdge(int square) { return (square & 7) == 7; }

inline bool isSquareOnRightEdge(int square) { return (square & 7) == 0; }
