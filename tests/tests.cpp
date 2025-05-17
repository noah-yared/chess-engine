#include "tests.hpp"
#include "utils.h"
#include "knights.h"
#include "sliding.h"
#include "evalBoards.hpp"
#include "evaluate.h"

#include <vector>
#include <unordered_set>
#include <string>
#include <array>
#include <functional>
#include <optional>
#include <sstream>

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
                        std::array<std::vector<char>, N> castlingPrivileges) {
  int cases = 0;
  int passedCases = 0, failedCases = 0;
  for (int i = 0; i < N; i++) {
    ChessBoard& sBoard = testBoards[i];

    /* may want to print board only for failed cases to reduce clutter */
    std::cout << "Testing move generator for the following board..." << std::endl;
    printStringifiedBoard(sBoard);

    ull bbs[12] = {0ULL}; 
    toBitboard(sBoard, bbs);
    int wKing = __builtin_ctzll(bbs[pieceIndex['K']]), bKing = __builtin_ctzll(bbs[pieceIndex['k']]);

    Board wBoard = Board(bbs, castlingPrivileges[i], enpassantWhite[i], bKing, wKing);
    Board bBoard = Board(bbs, castlingPrivileges[i], enpassantBlack[i], bKing, wKing);
      
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
        // std::cout << '\nExpected:\n' << printMoves(expectedWhite) << std::endl;
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
        // std::cout << '\nExpected:\n' << printMoves(expectedBlack) << std::endl;
        ++failedCases;
      }
    } else {
      std::cout << "SKIPPED!\n";
    }
  }
  if (!failedCases)
    std::cout << "ALL PASSED!\n\n";
  else
    std::cout << "PASSED " << passedCases << '/' << cases << " cases.\n\n";
}

void testAttackGeneration() {
  auto kingAttacks = compileKingAttacks();
  printAttacks(kingAttacks); // works

  auto knightAttacks = compileKnightAttacks();
  printAttacks(knightAttacks); // works

  auto board = Board();
  std::array<ull, 64> directionBM;
  for (int dir : {WEST, EAST, NORTH, SOUTH, NORTHEAST, SOUTHWEST, NORTHWEST, SOUTHEAST}) {
    for (int i = 0; i < 64; i++) 
      directionBM[i] = board.slidingAttacks()[i][dir];
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
    CASTLING_PRIVILEGES
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
    CASTLING_PRIVILEGES
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
    CASTLING_PRIVILEGES
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
    CASTLING_PRIVILEGES
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
    CASTLING_PRIVILEGES
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
    CASTLING_PRIVILEGES
  );
}

void testEvaluation() {
  using namespace Evaluation;
  Evaluator eval;
  testEvaluator<NUM_TESTS>(
    eval.evaluate, 
    TEST_BOARDS, 
    ENPASSANT,
    CASTLING_PRIVILEGES
  );
}

template<std::size_t N>
void testEvaluator(std::function<Evaluation::Score(const Board&)> evaluatingFunction,
                    std::array<ChessBoard, N> testBoards,
                    std::array<std::optional<int>, N> enpassant,
                    std::array<std::vector<char>, N> castlingPrivileges) {
  std::function<Evaluation::Score(const Board&)> evalFunc = evaluatingFunction;
  for (int i = 0; i < N; i++) {
    ChessBoard& sBoard = testBoards[i];

    ull bbs[12] = {0ULL}; 
    toBitboard(sBoard, bbs);
    int wKing = __builtin_ctzll(bbs[pieceIndex['K']]), bKing = __builtin_ctzll(bbs[pieceIndex['k']]);

    Board board = Board(bbs, castlingPrivileges[i], enpassant[i], bKing, wKing);

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