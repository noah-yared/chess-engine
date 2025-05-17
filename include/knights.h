#include <memory>
#include <vector>

#include "board.hpp"
#include "Move.h"

namespace Knight {
std::vector<std::unique_ptr<Move>> generateMoves(Board *, Side);
}
