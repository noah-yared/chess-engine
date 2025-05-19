#pragma once

#include <array>

const int MAX_DEPTH = 14;

class SearchPath {
  std::array<uint32_t, MAX_DEPTH> searchPath;
  int ply;

 public:
  SearchPath(): ply{0} {};
  void push(uint32_t state) { searchPath.at(ply++) = state; }
  uint32_t pop() { return searchPath.at(--ply); }
};

