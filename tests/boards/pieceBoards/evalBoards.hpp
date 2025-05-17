#ifndef EVAL_BOARDS_HPP
#define EVAL_BOARDS_HPP

#include <array>

using ChessBoard = std::array<std::array<char, 8>, 8>;
using ull = unsigned long long;

namespace Evaluation {
const std::size_t NUM_TESTS = 4;
std::array<std::optional<int>, NUM_TESTS> ENPASSANT;
std::array<std::vector<char>, NUM_TESTS> CASTLING_PRIVILEGES;
std::array<bool, NUM_TESTS> KING_IN_CHECK_WHITE = {{
  false, false, false, false
}};
std::array<bool, NUM_TESTS> KING_IN_CHECK_BLACK = {{
  false, false, false, true
}}; 
std::array<ull, NUM_TESTS> WHITE_ATTACKERS = {{
  0, 0, 0, 0
}};
std::array<ull, NUM_TESTS> BLACK_ATTACKERS = {{
  0, 0, 0, 0
}};




const std::array<ChessBoard, NUM_TESTS> TEST_BOARDS = {{
    // Starting position (should evaluate close to 0)
    {{
        {{'r','n','b','q','k','b','n','r'}},
        {{'p','p','p','p','p','p','p','p'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'P','P','P','P','P','P','P','P'}},
        {{'R','N','B','Q','K','B','N','R'}}
    }},

    // White material advantage (should be around +300)
    {{
        {{'r','n','b','q','k','b','n','r'}},
        {{'p','p','.','.','p','p','p','p'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','p','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'P','P','P','P','P','P','P','P'}},
        {{'R','N','B','Q','K','B','N','R'}}
    }},

    // White positional advantage (knights in center)
    {{
        {{'r','n','b','q','k','b','n','r'}},
        {{'p','p','p','p','p','p','p','p'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','N','N','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'P','P','P','P','P','P','P','P'}},
        {{'R','.','B','Q','K','B','.','R'}}
    }},

    // Black winning position (should be strongly negative)
    {{
        {{'.','.','.','.','k','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','q','r','.','.','.','p'}},
        {{'.','.','.','.','.','.','P','.'}},
        {{'.','.','.','.','.','.','.','.'}},
        {{'.','.','.','.','.','P','.','P'}},
        {{'.','.','.','.','K','.','.','.'}}
    }}
}};
}

#endif