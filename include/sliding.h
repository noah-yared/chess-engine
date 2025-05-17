#include <memory>
#include <vector>

#include "board.hpp"
#include "Move.h"
#include "directions.h"
#include "pieces.h"
#include "sides.h"

namespace Sliding {
std::vector<std::unique_ptr<Move>> getMovesAlongDirection(
    Board *, Side, int, Pieces::piece, int);
}  // namespace Sliding

namespace Rook {
std::vector<std::unique_ptr<Move>> generateMoves(Board *, Side);
}

namespace Bishop {
std::vector<std::unique_ptr<Move>> generateMoves(Board *, Side);
}

namespace Queen {
std::vector<std::unique_ptr<Move>> generateMoves(Board *, Side);
}
