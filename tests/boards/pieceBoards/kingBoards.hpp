#pragma once

#include <array>
#include <optional>
using ChessBoard = std::array<std::array<char, 8>, 8>;
using Tile = std::pair<int, int>;

namespace King{

std::array<std::optional<int>, 6> ENPASSANT_WHITE, ENPASSANT_BLACK;
std::array<std::vector<char>, 6> CASTLING_PRIVILEGES = {{
  {'k', 'q', 'K', 'Q'}, {'k', 'q', 'K', 'Q'}, {}, {}, {}, {},
}};
std::array<bool, 6> KING_IN_CHECK_WHITE = {{
  false, false, false, false, false, false
}};
std::array<bool, 6> KING_IN_CHECK_BLACK = {{
  false, false, false, false, false, false
}};
std::array<ull, 6> WHITE_ATTACKERS;
std::array<ull, 6> BLACK_ATTACKERS;

// testing: castling, escaping/avoiding check, capturing (one king in check at a time)
// if one side is in check, only compute moves for side in check
std::array<ChessBoard, 6> TEST_BOARDS = {{
  /*
   * attacked squares to avoid
   */
  {{{{'r', 'n', 'b', 'q', 'k', 'b', '.', 'r'}},
    {{'p', 'p', 'p', 'p', '.', '.', 'p', 'p'}},
    {{'.', '.', '.', '.', '.', 'n', '.', '.'}},
    {{'.', '.', '.', 'N', 'p', '.', '.', 'q'}},
    {{'.', '.', '.', '.', 'P', 'P', '.', '.'}},
    {{'.', '.', 'P', 'Q', '.', '.', '.', '.'}},
    {{'P', 'P', '.', '.', '.', '.', 'P', 'P'}},
    {{'R', '.', 'B', '.', 'K', 'B', 'N', 'R'}}}},

  /*
   * attacked squares king has to avoid, 
   * lines of attack blocked, captures,
   * available castling (7, 6)
   */
  {{{{'r', '.', 'b', '.', 'k', '.', '.', 'r'}},
    {{'p', 'p', 'p', '.', 'b', 'N', 'p', 'p'}},
    {{'.', '.', '.', '.', '.', 'n', '.', '.'}},
    {{'.', '.', '.', '.', 'p', '.', '.', 'q'}},
    {{'.', '.', '.', '.', 'P', 'P', '.', '.'}},
    {{'.', '.', 'P', 'Q', '.', 'N', '.', '.'}},
    {{'P', 'P', '.', 'B', 'n', '.', 'P', 'P'}},
    {{'R', '.', '.', '.', 'K', 'B', '.', 'R'}}}},

  /*
   * white king is stuck and in check, 
   * cannot escape check on own,
   * >>TEST WHITE ONLY<<
   */
  {{{{'.', 'k', 'r', '.', '.', '.', '.', 'b'}},
    {{'p', 'p', 'p', '.', '.', '.', '.', 'b'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'N', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'P', '.', '.', '.', '.', '.', '.', '.'}},
    {{'K', '.', 'R', '.', 'r', '.', '.', '.'}}}},

  /*
   * black king is stuck, no captures, 
   * no attacks, white king can only retreat
   */
  {{{{'.', '.', '.', '.', '.', '.', '.', 'k'}},
    {{'.', '.', '.', '.', '.', '.', '.', 'P'}},
    {{'.', '.', '.', '.', '.', '.', '.', 'K'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}}}},  

  /*
   * black king is in check, only one capture 
   * move to escape
   * >>TEST BLACK ONLY<< 
   */
  {{{{'.', 'k', 'R', '.', '.', '.', '.', '.'}},
    {{'b', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', 'p', '.', '.', '.', '.', '.', '.'}},
    {{'.', 'P', 'p', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', 'B', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', 'R', 'K', '.', '.', '.', '.'}}}},

  /*
   * both kings smothered by friendlies,
   * no legal moves for either
   */
  {{{{'n', 'k', 'r', '.', '.', '.', '.', '.'}},
    {{'p', 'p', 'p', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', '.', '.', '.'}},
    {{'.', '.', '.', '.', '.', 'P', 'P', 'P'}},
    {{'.', '.', '.', '.', '.', 'R', 'K', 'N'}}}},
}};


std::array<std::vector<std::pair<Tile, Tile>>, 6> WHITE_SOLS = {{
  {{{0, 4}, {1, 3}}, {{0, 4}, {1, 5}}},
  {{{0, 4}, {0, 3}}, {{0, 4}, {1, 4}}, {{0, 4}, {1, 5}}},
  {},
  {{{5, 7}, {5, 6}}, {{5, 7}, {4, 6}}, {{5, 7}, {4, 7}}},
  {},
  {},
}};

std::array<std::vector<std::pair<Tile, Tile>>, 6> BLACK_SOLS = {{
  {{{7, 4}, {6, 5}}},
  {{{7, 4}, {6, 5}}, {{7, 4}, {7, 5}}, {{7, 4}, {7, 6}}},
  {},
  {},
  {{{7, 1}, {7, 2}}},
  {},
}};

std::array<bool, 6> TESTING_WHITE = {{
  true, true, true, true, false, true,
}};

std::array<bool, 6> TESTING_BLACK = {{
  true, true, false, true, true, true,
}};

constexpr std::size_t NUM_TESTS = TEST_BOARDS.size();

}