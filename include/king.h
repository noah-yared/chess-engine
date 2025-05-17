#ifndef KING_H
#define KING_H

#include <array>
#include <memory>
#include <vector>

#include "board.hpp"
#include "Move.h"
#include "attacks.h"
#include "directions.h"
#include "pieces.h"
#include "sides.h"

namespace King {
std::vector<std::unique_ptr<Move>> generateMoves(Board *, Side);
}

#endif