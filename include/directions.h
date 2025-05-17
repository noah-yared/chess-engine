#ifndef DIRECTIONS_H
#define DIRECTIONS_H

/* Direction macros */
#define WEST 0
#define EAST 1
#define NORTH 2
#define SOUTH 3
#define NORTHEAST 4
#define SOUTHWEST 5
#define NORTHWEST 6
#define SOUTHEAST 7


namespace Directions {
inline int reverse(int direction) {
  return direction - 2 * (direction & 1) + 1;
} 

inline bool isDiagonal(int direction) {
  return direction >= 4 && direction <= 7;
}
}

#endif