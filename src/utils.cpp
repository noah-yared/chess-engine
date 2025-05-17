#include "utils.h"

#include <memory>
#include <iostream>

void printMove(const std::unique_ptr<Move>& move) {
  if (move)
    std::cout << (move->i() / 8) << ',' << (7 - move->i() % 8) << ' ' << (move->f() / 8) << ',' << (7 - move->f() % 8) << ' ' << move->p() << std::endl;
  else
    std::cerr << "Move pointer is null!" << std::endl;
}

void printMove(const Move* move) {
  if (move)
    std::cout << (move->i() / 8) << ',' << (7 - move->i() % 8) << ' ' << (move->f() / 8) << ',' << (7 - move->f() % 8) << ' ' << move->p() << std::endl;
  else
    std::cerr << "Move pointer is null!" << std::endl;
}

void printMove(const Move& move) {
  if (move.i() != move.f())
    std::cout << (move.i() / 8) << ',' << (7 - move.i() % 8) << ' ' << (move.f() / 8) << ',' << (7 - move.f() % 8) << ' ' << move.p() << std::endl;
  else
    std::cerr << "Move pointer is null!" << std::endl;
}

ull* toBitboard(std::array<std::array<char, 8>, 8>& stringifiedBoard, ull *bbs) {
  auto pieceIndex = std::unordered_map<char, int> { /* map pieces to bb index*/
    {'p', 0}, {'P', 6},
    {'r', 1}, {'R', 7},
    {'n', 2}, {'N', 8}, 
    {'b', 3}, {'B', 9},
    {'q', 4}, {'Q', 10},
    {'k', 5}, {'K', 11}
  };
  ull *bb = bbs, mask = 1ULL; 
  for (auto outer_rit = stringifiedBoard.rbegin(); outer_rit != stringifiedBoard.rend(); ++outer_rit) {
    for (auto rit = outer_rit->rbegin(); rit != outer_rit->rend(); ++rit) {
      if (*rit != '.') bbs[pieceIndex[*rit]] |= mask; /* piece at square */
      mask <<= 1;
    }
  }
  return bbs;
}
