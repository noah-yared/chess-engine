#ifndef KINGSAFETY_H
#define KINGSAFETY_H

#include <vector>

#include "board.hpp"
#include "directions.h"
#include "sides.h"

int getBitIncrement(int, bool);
bool doesMoveExposeAllyKingToCheck(Board* board, Move* move, Side side);

namespace Attack {
bool doesMovePutOpponentKingInCheck(Board* board, Move* move, Side side);
bool isSquareUnattacked(Board* board, int square, Side side);
}

#endif