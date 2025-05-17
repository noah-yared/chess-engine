#ifndef KINGSAFETY_H
#define KINGSAFETY_H

#include <vector>

#include "board.hpp"
#include "directions.h"
#include "sides.h"

int getBitIncrement(int, bool);

namespace Attack {
bool doesMovePutOpponentKingInCheck(Board*, Move*, Side);
bool doesMoveExposeAllyKingToCheck(Board*, Move*, Side);
bool doesMovePutKingInCheck(Board*, Move*, Side);
bool isSquareUnattacked(Board*, int, Side);
bool isKingAttackingSquare(Board*, int, Side);
bool isKnightAttackingSquare(Board*, int, Side);
bool isSlidingPieceAttackingSquare(Board*, int, Side);
bool isPawnAttackingSquare(Board*, int, Side);
}

#endif