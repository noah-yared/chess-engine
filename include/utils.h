#ifndef UTILS_H
#define UTILS_H

#include "Move.h"
#include <memory>

using ull = unsigned long long;

inline bool inBoard(int x, int y) {
  // return (x | y) < 8; // fails because x,y signed
  return !((x | y) & ~7);
}

void printMove(const std::unique_ptr<Move>& move);
void printMove(const Move* move);
void printMove(const Move& board);

ull* toBitboard(std::array<std::array<char, 8>, 8>& stringifiedBoard, ull *bbs);

#endif