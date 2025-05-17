#include "utils.h"
#include "Move.h"
#include "board.hpp"

#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

void printMove(const std::unique_ptr<Move>& move) {
  printMove(move.get());
}

void printMove(const Move* move) {
  if (move) {
    std::cout << (move->i() / 8) << ',' << (7 - move->i() % 8) << ' ' << (move->f() / 8) << ',' << (7 - move->f() % 8) << ' ' << move->p() << std::endl;
    // std::cout << stringify(move) << std::endl;
  }
  else
    std::cerr << "Move pointer is null!" << std::endl;
}

void printMove(const Move& move) {
  if (move.i() != move.f()) {
    std::cout << (move.i() / 8) << ',' << (7 - move.i() % 8) << ' ' << (move.f() / 8) << ',' << (7 - move.f() % 8) << ' ' << move.p() << std::endl;
    // std::cout << stringify(&move) << std::endl;
  }
  else
    std::cerr << "Move pointer is null!" << std::endl;
}

void printMoves(Moves& moves) {
  for (auto& move : moves)
    std::cout << stringify(move.get()) << std::endl;
}

void printMoves(std::vector<std::pair<Tile, Tile>>& moves)  {
  for (auto& move : moves)
    std::cout << stringify(move) << std::endl;
}

void printBitmap(ull bitmap) {
  ull mask = 0xFFULL;
  for (int row = 0; row < 8; row++)
    std::cout << std::bitset<8>((bitmap >> (56-8*row)) & mask).to_string() << '\n';
}

void printAttacks(std::array<ull, 64> attacks) {
  for (ull bm : attacks) { printBitmap(bm); std::cout << '\n'; }
}  

void printStringifiedBoard(std::array<std::array<char, 8>, 8>& board) {
  for (auto row : board) {
    for (char c : row) std::cout << c;
    std::cout << '\n';
  }
}

void printBoard(const Board& board) {
  auto pieceIndex = std::unordered_map<char, int> { /* map pieces to bb index*/
    {'p', 0}, {'P', 6},
    {'r', 1}, {'R', 7},
    {'n', 2}, {'N', 8}, 
    {'b', 3}, {'B', 9},
    {'q', 4}, {'Q', 10},
    {'k', 5}, {'K', 11}
  };

  std::array<std::array<char, 8>, 8> stringifiedBoard; 
  std::array<char, 8> defaultRow; defaultRow.fill('.');
  stringifiedBoard.fill(defaultRow);
  for (auto it = pieceIndex.begin(); it != pieceIndex.end(); it++) {
    std::string bitstring = std::bitset<64>(board.readBB(static_cast<Pieces::piece>(it->second))).to_string();
    for (int i = 0; i < 64; i++) {
      if (bitstring[i] == '1') {
        stringifiedBoard[7 - (i / 8)][i % 8] = it->first;
      }
    }
  }
  printStringifiedBoard(stringifiedBoard);
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

  ull mask = 1ULL; 
  for (auto outer_rit = stringifiedBoard.rbegin(); outer_rit != stringifiedBoard.rend(); ++outer_rit) {
    for (auto rit = outer_rit->rbegin(); rit != outer_rit->rend(); ++rit) {
      if (*rit != '.') bbs[pieceIndex[*rit]] |= mask; /* piece at square */
      mask <<= 1;
    }
  }
  return bbs;
}

std::string stringify(Tile tile) {
  std::stringstream ss;
  ss << '(' << tile.first << ", " << tile.second << ')';
  return ss.str();
}

std::string stringify(int tile) {
  std::stringstream ss;
  ss << '(' << (tile / 8) << ", " << (7 - (tile % 8)) << ')';
  return ss.str();
}

std::string stringify(const Move* move) {
  std::stringstream ss;
  ss << '(' << stringify(move->i()) << ", " << stringify(move->f()) <<')';
  return ss.str();
}

std::string stringify(std::pair<Tile, Tile> move) {
  std::stringstream ss;
  ss << '(' << stringify(move.first) << ", " << stringify(move.second) << ") ";
  return ss.str();
}

std::string hashMove(std::pair<Tile, Tile> move) {
  std::stringstream ss;
  Tile tile1 = move.first, tile2 = move.second;
  ss << tile1.first << tile1.second << tile2.first << tile2.second;
  return ss.str();
}

std::string hashMove(std::unique_ptr<Move>& move) {
  std::stringstream ss;
  ss << (move->i() / 8) << (7 - move->i() % 8) << (move->f() / 8) << (7 - move->f() % 8);
  return ss.str(); 
}
