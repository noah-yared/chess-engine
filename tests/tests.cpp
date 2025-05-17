#include "tests.hpp"
#include "knights.h"
#include "sliding.h"
#include "evalBoards.hpp"
#include "evaluate.h"


using ull = unsigned long long;
using Moves = std::vector<std::unique_ptr<Move>>;
using Tile = std::pair<int, int>;


std::unordered_map<char, int> pieceIndex { /* map pieces to bb index*/
  {'p', 0}, {'P', 6},
  {'r', 1}, {'R', 7},
  {'n', 2}, {'N', 8},
  {'b', 3}, {'B', 9},
  {'q', 4}, {'Q', 10},
  {'k', 5}, {'K', 11}
};

void printBitmap(ull bitmap) {
  ull mask = 0xFFULL;
  for (int row = 0; row < 8; row++)
    std::cout << std::bitset<8>((bitmap >> (56-8*row)) & mask).to_string() << '\n';
}

void printAttacks(std::array<ull, 64> attacks) {
  for (ull bm : attacks) { printBitmap(bm); std::cout << '\n'; }
}  

ull* toBitboard(std::array<std::array<char, 8>, 8>& stringifiedBoard, ull *bbs) {
  ull *bb = bbs, mask = 1ULL; 
  for (auto outer_rit = stringifiedBoard.rbegin(); outer_rit != stringifiedBoard.rend(); ++outer_rit) {
    for (auto rit = outer_rit->rbegin(); rit != outer_rit->rend(); ++rit) {
      if (*rit != '.') bbs[pieceIndex[*rit]] |= mask; /* piece at square */
      mask <<= 1;
    }
  }
  return bbs;
}

void printStringifiedBoard(std::array<std::array<char, 8>, 8>& board) {
  for (auto row : board) {
    for (char c : row) std::cout << c;
    std::cout << '\n';
  }
}

void printBoard(const Board& board) {
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

std::string stringify(const std::unique_ptr<Move>& move) {
  std::stringstream ss;
  ss << '(' << stringify(move->i()) << ", " << stringify(move->f()) <<')';
  return ss.str();
}

std::string stringify(std::pair<Tile, Tile> move) {
  std::stringstream ss;
  ss << '(' << stringify(move.first) << ", " << stringify(move.second) << ") ";
  return ss.str();
}

void printMove(const std::unique_ptr<Move>& move) {
  if (move)
    std::cout << stringify(move) << std::endl;
  else
    std::cerr << "Move pointer is null!" << std::endl;
}

void printMoves(Moves& moves) {
  for (auto& move : moves)
    std::cout << stringify(move) << std::endl;
}

void printMoves(std::vector<std::pair<Tile, Tile>>& moves)  {
  for (auto& move : moves)
    std::cout << stringify(move) << std::endl;
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

bool matches(Moves& result, std::vector<std::pair<Tile, Tile>> expected) {
  if (result.size() != expected.size()) /* mismatching number of moves generated */
    return false;
  std::unordered_set<std::string> expectedSet;
  for (auto move : expected) /* build a set of expected results */
    expectedSet.insert(hashMove(move));
  for (auto& move : result)
    if (!expectedSet.count(hashMove(move))) return false;
  return true;
} 

/* POTENTIAL IMPROVEMENT: Aggregate parameters/input into a struct to reduce clutter (pass in pointer to struct) */
template<std::size_t N>
void testMoveGeneration(std::function<Moves(Board*, Side)> generatingFunction,
                        std::array<ChessBoard, N> testBoards,
                        std::array<std::vector<std::pair<Tile, Tile>>, N> expectedWhite, 
                        std::array<std::vector<std::pair<Tile, Tile>>, N> expectedBlack,
                        std::array<bool, N> testingWhite,
                        std::array<bool, N> testingBlack,
                        std::array<std::optional<int>, N> enpassantWhite,
                        std::array<std::optional<int>, N> enpassantBlack,
                        std::array<std::vector<char>, N> castlingPrivileges,
                        std::array<bool, N> kingInCheckWhite,
                        std::array<bool, N> kingInCheckBlack,
                        std::array<ull, N> whiteAttackers,
                        std::array<ull, N> blackAttackers) {
  int cases = 0;
  int passedCases = 0, failedCases = 0;
  for (int i = 0; i < N; i++) {
    // if (i != 1) continue;

    ChessBoard& sBoard = testBoards[i];

    /* may want to print board only for failed cases to reduce clutter */
    std::cout << "Testing move generator for the following board..." << std::endl;
    printStringifiedBoard(sBoard);

    ull bbs[12] = {0ULL}; 
    toBitboard(sBoard, bbs);
    int wKing = __builtin_ctzll(bbs[pieceIndex['K']]), bKing = __builtin_ctzll(bbs[pieceIndex['k']]);

    // std::cout << "White king location: " << wKing << " or " << stringify(wKing) 
    //           << "\nBlack king location: " << bKing << " or " << stringify(bKing) << '\n';

    Board wBoard = Board(bbs, castlingPrivileges[i], enpassantWhite[i], bKing, wKing,
                              kingInCheckWhite[i], kingInCheckBlack[i], whiteAttackers[i], blackAttackers[i]);
    Board bBoard = Board(bbs, castlingPrivileges[i], enpassantBlack[i], bKing, wKing,
                              kingInCheckWhite[i], kingInCheckBlack[i], whiteAttackers[i], blackAttackers[i]);
      
    std::cout << "Testing white... ";
    if (testingWhite[i] && ++cases) {
      auto resultWhite = generatingFunction(&wBoard, Side::WHITE);
      if (matches(resultWhite, expectedWhite[i])) {
        std::cout << "PASSED!\n";
        ++passedCases;
      } else {
        std::cout << "FAILED!\nGot:\n";
        printMoves(resultWhite);
        std::cout << "\nExpected:\n";
        printMoves(expectedWhite[i]);
        std::cout << std::endl;
        // << '\nExpected:\n' << printMoves(expectedWhite) << std::endl;
        ++failedCases;
      }
    } else {
      std::cout << "SKIPPED!\n";
    }

    std::cout << "Testing black... ";
    if (testingBlack[i] && ++cases) {
      auto resultBlack = generatingFunction(&bBoard, Side::BLACK);
      if (matches(resultBlack, expectedBlack[i])) {
        std::cout << "PASSED!\n";
        ++passedCases;
      } else {
        std::cout << "FAILED!\nGot:\n";
        printMoves(resultBlack);
        std::cout << "\nExpected:\n";
        printMoves(expectedBlack[i]);
        std::cout << std::endl;
        // << '\nExpected:\n' << printMoves(expectedBlack) << std::endl;
        ++failedCases;
      }
    } else {
      std::cout << "SKIPPED!\n";
    }
  }
  if (!failedCases)
    std::cout << "ALL PASSED!" << std::endl;
  else
    std::cout << "PASSED " << passedCases << '/' << cases << " cases.\n";
}

void testAttackGeneration() {
  auto kingAttacks = compileKingAttacks();
  printAttacks(kingAttacks); // works

  auto knightAttacks = compileKnightAttacks();
  printAttacks(knightAttacks); // works

  auto slidingAttacks = compileSlidingAttacks(); // works
  std::array<ull, 64> directionBM;
  for (int dir = 6; dir < 7; dir++) {
    for (int i = 0; i < 64; i++) 
      directionBM[i] = slidingAttacks[i][dir];
    printAttacks(directionBM);
    std::cout << '\n';
  }
}

void testKingMoveGeneration() {
  using namespace King;
  std::function<Moves(Board*, Side)> genFunc = generateMoves;
  testMoveGeneration<NUM_TESTS>(
    genFunc,
    TEST_BOARDS, 
    WHITE_SOLS, 
    BLACK_SOLS,
    TESTING_WHITE,
    TESTING_BLACK,
    ENPASSANT_WHITE,
    ENPASSANT_BLACK,
    CASTLING_PRIVILEGES,
    KING_IN_CHECK_WHITE,
    KING_IN_CHECK_BLACK,
    WHITE_ATTACKERS,
    BLACK_ATTACKERS
  );
}

void testPawnMoveGeneration() {
  using namespace Pawn;
  std::function<Moves(Board*, Side)> genFunc = generateMoves;
  testMoveGeneration<NUM_TESTS>(
    genFunc,
    TEST_BOARDS, 
    WHITE_SOLS, 
    BLACK_SOLS,
    TESTING_WHITE,
    TESTING_BLACK,
    ENPASSANT_WHITE,
    ENPASSANT_BLACK,
    CASTLING_PRIVILEGES,
    KING_IN_CHECK_WHITE,
    KING_IN_CHECK_BLACK,
    WHITE_ATTACKERS,
    BLACK_ATTACKERS
  );

}

void testBishopMoveGeneration() {
  using namespace Bishop;
  std::function<Moves(Board*, Side)> genFunc = generateMoves;
  testMoveGeneration<NUM_TESTS>(
    genFunc,
    TEST_BOARDS, 
    WHITE_SOLS, 
    BLACK_SOLS,
    TESTING_WHITE,
    TESTING_BLACK,
    ENPASSANT_WHITE,
    ENPASSANT_BLACK,
    CASTLING_PRIVILEGES,
    KING_IN_CHECK_WHITE,
    KING_IN_CHECK_BLACK,
    WHITE_ATTACKERS,
    BLACK_ATTACKERS
  );

}

void testKnightMoveGeneration() {
  using namespace Knight;
  std::function<Moves(Board*, Side)> genFunc = generateMoves;
  testMoveGeneration<NUM_TESTS>(
    genFunc,
    TEST_BOARDS, 
    WHITE_SOLS, 
    BLACK_SOLS,
    TESTING_WHITE,
    TESTING_BLACK,
    ENPASSANT_WHITE,
    ENPASSANT_BLACK,
    CASTLING_PRIVILEGES, 
    KING_IN_CHECK_WHITE,
    KING_IN_CHECK_BLACK,
    WHITE_ATTACKERS,
    BLACK_ATTACKERS
  );

} 

void testQueenMoveGeneration() {
  using namespace Queen;
  std::function<Moves(Board*, Side)> genFunc = generateMoves;
  testMoveGeneration<NUM_TESTS>(
    genFunc,
    TEST_BOARDS, 
    WHITE_SOLS, 
    BLACK_SOLS,
    TESTING_WHITE,
    TESTING_BLACK,
    ENPASSANT_WHITE,
    ENPASSANT_BLACK,
    CASTLING_PRIVILEGES,
    KING_IN_CHECK_WHITE,
    KING_IN_CHECK_BLACK,
    WHITE_ATTACKERS,
    BLACK_ATTACKERS
  );

}

void testRookMoveGeneration() {
  using namespace Rook;
  std::function<Moves(Board*, Side)> genFunc = generateMoves;
  testMoveGeneration<NUM_TESTS>(
    genFunc,
    TEST_BOARDS, 
    WHITE_SOLS, 
    BLACK_SOLS,
    TESTING_WHITE,
    TESTING_BLACK,
    ENPASSANT_WHITE,
    ENPASSANT_BLACK,
    CASTLING_PRIVILEGES,
    KING_IN_CHECK_WHITE,
    KING_IN_CHECK_BLACK,
    WHITE_ATTACKERS,
    BLACK_ATTACKERS
  );

}

void testEvaluation() {
  using namespace Evaluation;
  Evaluator eval;
  testEvaluator<NUM_TESTS>(
    eval.evaluate, 
    TEST_BOARDS, 
    ENPASSANT,
    CASTLING_PRIVILEGES, 
    KING_IN_CHECK_WHITE, 
    KING_IN_CHECK_BLACK, 
    WHITE_ATTACKERS, 
    BLACK_ATTACKERS
  );
}

template<std::size_t N>
void testEvaluator(std::function<Evaluation::Score(const Board&)> evaluatingFunction,
                    std::array<ChessBoard, N> testBoards,
                    std::array<std::optional<int>, N> enpassant,
                    std::array<std::vector<char>, N> castlingPrivileges,
                    std::array<bool, N> kingInCheckWhite,
                    std::array<bool, N> kingInCheckBlack,
                    std::array<ull, N> whiteAttackers,
                    std::array<ull, N> blackAttackers) {

  std::function<Evaluation::Score(const Board&)> evalFunc = evaluatingFunction;
  for (int i = 0; i < N; i++) {
    ChessBoard& sBoard = testBoards[i];

    ull bbs[12] = {0ULL}; 
    toBitboard(sBoard, bbs);
    int wKing = __builtin_ctzll(bbs[pieceIndex['K']]), bKing = __builtin_ctzll(bbs[pieceIndex['k']]);

    Board board = Board(bbs, castlingPrivileges[i], enpassant[i], bKing, wKing,
                              kingInCheckWhite[i], kingInCheckBlack[i], whiteAttackers[i], blackAttackers[i]);

    std::cout << "Evaluating board:\n";
    printStringifiedBoard(sBoard);
    std::cout << "Evaluation: " << evalFunc(board) << '\n';
  }
}

// template<std::size_t N>
// void testMakeMove(std::function<Evaluation::Score(const Board&)> evaluatingFunction,
//                       std::array<ChessBoard, N> testBoards,
//                       std::array<std::vector<Move>, N> moves, 
//                       std::array<ChessBoard, N> expectedBoards,
//                       std::array<std::optional<int>, N> enpassant,
//                       std::array<std::vector<char>, N> castlingPrivileges,
//                       std::array<bool, N> kingInCheckWhite,
//                       std::array<bool, N> kingInCheckBlack,
//                       std::array<ull, N> whiteAttackers,
//                       std::array<ull, N> blackAttackers) {
//   std::function<Evaluation::Score(const Board&) > evalFunc = evaluatingFunction;
//   std::cout << "Testing makeMove and undoMove..." << std::endl;
//   int passedCases = 0, failedCases = 0;
//   for (int i = 0; i < N; i++) {
//     ChessBoard& sBoard = testBoards[i];

//     ull bbs[12] = {0ULL}; 
//     toBitboard(sBoard, bbs);
//     int wKing = __builtin_ctzll(bbs[pieceIndex['K']]), bKing = __builtin_ctzll(bbs[pieceIndex['k']]);

//     std::cout << "Testing board:\n";
//     printStringifiedBoard(sBoard);

//     Board board = Board(bbs, castlingPrivileges[i], enpassant[i], bKing, wKing,
//                               kingInCheckWhite[i], kingInCheckBlack[i], whiteAttackers[i], blackAttackers[i]);
//     for (const auto& move : moves[i]) {
//       board.makeMove(&move);
//       std::cout << "Move: "; printMove(move);
//       std::cout << "Castle: " << (move.isCastle() ? "true" : "false") << std::endl
//                 << "Enpassant: " << (move.isEnpassant() ? "true" : "false") << std::endl
//                 << "Promotion: " << (move.isPromotion() ? "true" : "false") << std::endl;
//       std::cout << "Board after move:\n"; printBoard(board);
//       if (board == expectedBoards[i]) {
//         std::cerr << "Incorrect board after move!" << std::endl << "Expected board:\n"; printBoard(expectedBoards[i]);
//         exit(-1);
//       } 
//     }
//   }
// }

int main() {
  // testAttackGeneration();
  testKingMoveGeneration();
  testPawnMoveGeneration();
  testBishopMoveGeneration();
  testKnightMoveGeneration();
  testRookMoveGeneration();
  testQueenMoveGeneration();
  // testEvaluation();
  return 0;
}