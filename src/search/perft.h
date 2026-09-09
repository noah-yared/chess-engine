#pragma once

#include "board/position.h"
#include "move/move_generator.h"

template <Color color>
u64 perft(Position& pos, int depth)
{
    MoveList moves{};
    MoveGenerator::pushLegalMoves<color>(pos, moves);
    if (depth == 1)
        return moves.size();
    u64 nodeCount = 0ULL;
    for (auto& move : moves)
    {
        auto snapshot = pos.getStateSnapshot();
        pos.applyMove(move);
        nodeCount += perft<opposite<color>()>(pos, depth - 1);
        pos.undoMove(move, snapshot);
    }
    return nodeCount;
}
