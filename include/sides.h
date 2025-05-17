#ifndef SIDES_H
#define SIDES_H

#include <algorithm>

#include "pieces.h"

enum Side { BLACK, WHITE };

inline Side getSide(Pieces::piece p) {
  return p >= Pieces::piece::P ? WHITE : BLACK;
}

inline Side getOppSide(Pieces::piece p) {
  return p >= Pieces::piece::P ? BLACK : WHITE;
}

#endif
