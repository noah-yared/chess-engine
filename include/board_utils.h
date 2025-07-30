#pragma once

#include <string>
#include <sstream>

#include "constants.h"
#include "platform.h"
#include "position.h"


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
