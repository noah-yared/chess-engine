#ifndef PAWNS_H
#define PAWNS_H

#include <memory>
#include <vector>
#include <optional>
#include "board.hpp"
#include "Move.h"
#include "sides.h"

typedef unsigned long long ull;

namespace Pawn {
std::optional<std::pair<std::unique_ptr<Move>, std::unique_ptr<Move>>> getEnpassantMoves(Board *, Side);
std::vector<std::unique_ptr<Move>> getDoubleSteps(Board *, Side);
std::vector<std::unique_ptr<Move>> getSingleSteps(Board *, Side);
std::vector<std::unique_ptr<Move>> getDiagonalAttacks(Board *, Side);
std::vector<std::unique_ptr<Move>> generateMoves(Board *, Side);
}  // namespace Pawn

#endif