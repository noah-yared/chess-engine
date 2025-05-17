#ifndef UTILS_H
#define UTILS_H

#include "Move.h"
#include "board.hpp"

#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>

using ull = unsigned long long;
using Moves = std::vector<std::unique_ptr<Move>>;
using Tile = std::pair<int, int>;

inline auto pieceIndex = std::unordered_map<char, int> { /* map pieces to bb index*/
  {'p', 0}, {'P', 6},
  {'r', 1}, {'R', 7},
  {'n', 2}, {'N', 8}, 
  {'b', 3}, {'B', 9},
  {'q', 4}, {'Q', 10},
  {'k', 5}, {'K', 11}
};

inline bool inBoard(int x, int y) {
  // return (x | y) < 8; // fails because x,y signed
  return !((x | y) & ~7);
}

void printMove(const std::unique_ptr<Move>&);
void printMove(const Move*);
void printMove(const Move&);

void printMoves(Moves& moves);
void printMoves(std::vector<std::pair<Tile, Tile>>& moves);

void printBitmap(ull bitmap);

void printAttacks(std::array<ull, 64> attacks);

void printStringifiedBoard(std::array<std::array<char, 8>, 8>& board);

void printBoard(const Board& board);

ull* toBitboard(std::array<std::array<char, 8>, 8>& stringifiedBoard, ull *bbs);

std::string stringify(Tile);
std::string stringify(int);
std::string stringify(const Move*);
std::string stringify(std::pair<Tile, Tile>);

std::string hashMove(std::pair<Tile, Tile> move);
std::string hashMove(std::unique_ptr<Move>& move);

#endif