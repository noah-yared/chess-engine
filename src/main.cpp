#include <bitset>
#include <iostream>

#include "engine.h"

void printBitboard(u64 bb) {
  for (int r = 7; r >= 0; --r) {
    for (int c = 7; c >= 0; --c) {
      int sq = r * FILES + c;
      if (bb & (1ULL << sq)) {
        std::cout << "X";
      } else {
        std::cout << ".";
      }
    }
    std::cout << "\n";
  }
}

int main() {
  // check to see if engine calculateBestMove is mutating the position object
  // by printing board after each execution of calculateBestMove
  SearchEngine engine;
  std::cout << "Starting position:\n";
  engine.dumpPosition();
  engine.search();
  std::cout << "After calculation:\n";
  engine.dumpPosition();

  return 0;
}
