#pragma once

#include <array>

#include "constants.h"
#include "pieces.h"
#include "platform.h"

// Directions are from the perspective of white
enum class Direction
{
    W,
    E,
    N,
    S,
    NE,
    SW,
    NW,
    SE,
};

inline Direction ForwardDirection(Color c) noexcept
{
    constexpr std::array<Direction, 2> forwardDirections = {
        Direction::S,
        Direction::N,
    };
    return forwardDirections[c == Color::WHITE];
}

class Directions
{
  private:
    // corresponds to sfamt for left shift
    inline static constexpr std::array<int, static_cast<int>(NUM_DIRECTIONS)> directionShift = {
        {1, -1, 8, -8, 7, -7, 9, -9}};
    inline static constexpr std::array<Direction, static_cast<int>(NUM_DIRECTIONS)>
        directionReverse = {{Direction::E, Direction::W, Direction::S, Direction::N, Direction::SW,
                             Direction::NE, Direction::SE, Direction::NW}};

  public:
    static constexpr std::pair<Direction, Direction> pawnAttackLanes(Color c) noexcept
    {
        constexpr std::array<std::pair<Direction, Direction>, 2> attackDirections = {
            std::pair{Direction::SW, Direction::SE},
            std::pair{Direction::NW, Direction::NE},
        };
        return attackDirections[c == Color::WHITE];
    }
    static constexpr Direction forwardLane(Color c) noexcept
    {
        return c == Color::WHITE ? Direction::N : Direction::S;
    }
    static constexpr Direction reverse(Direction d) noexcept
    {
        return directionReverse[static_cast<int>(d)];
    }
    static constexpr bool isDiagonal(Direction d) noexcept
    {
        return d >= Direction::NE && d <= Direction::SE;
    }
    static constexpr bool isUpwards(Direction d) noexcept { return sfamt(d) > 0; }
    static constexpr int sfamt(Direction d) noexcept { return directionShift[static_cast<int>(d)]; }
    static constexpr int numSqsInLane(Direction d, int square) noexcept
    {
        int rowIndex{7 - square / 8}, colIndex{7 - square % 8};
        switch (d)
        {
        case Direction::W:
            return colIndex;
        case Direction::E:
            return 7 - colIndex;
        case Direction::N:
            return rowIndex;
        case Direction::S:
            return 7 - rowIndex;
        case Direction::NE:
            return std::min(rowIndex, 7 - colIndex);
        case Direction::SW:
            return std::min(7 - rowIndex, colIndex);
        case Direction::NW:
            return std::min(rowIndex, colIndex);
        case Direction::SE:
            return std::min(7 - rowIndex, 7 - colIndex);
        default:
            return 0; // Instead of throwing, return 0 for invalid directions
        }
    }
};
