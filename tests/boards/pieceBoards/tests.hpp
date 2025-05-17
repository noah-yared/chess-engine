#ifndef TESTS_HPP
#define TESTS_HPP

#include <iostream>
#include <bitset>
#include <string>
#include <unordered_map>
#include <functional>
#include <unordered_set>
#include <sstream>

#include "attacks.h"  
#include "bitboard.h"
#include "utils.h"
#include "directions.h"
#include "Move.h"
#include "board.hpp"
#include "kingSafety.h"
#include "king.h"
#include "pawns.h"
#include "sliding.h"
#include "pawnBoards.hpp"
#include "queenBoards.hpp"
#include "rookBoards.hpp"
#include "bishopBoards.hpp"
#include "knightBoards.hpp"
#include "kingBoards.hpp"
#include "evalBoards.hpp"
#include "evaluate.h"

using ull = unsigned long long;
using Moves = std::vector<std::unique_ptr<Move>>;
using Tile = std::pair<int, int>;

// Function declarations
void printBitmap(ull bitmap);
void printAttacks(std::array<ull, 64> attacks);
ull* toBitboard(std::array<std::array<char, 8>, 8>& stringifiedBoard, ull *bbs);
void printStringifiedBoard(std::array<std::array<char, 8>, 8>& board);
std::string stringify(Tile tile);
std::string stringify(int tile);
std::string stringify(const std::unique_ptr<Move>& move);
std::string stringify(std::pair<Tile, Tile> move);
void printMove(const std::unique_ptr<Move>& move);
void printMoves(std::vector<std::pair<Tile, Tile>>& moves);
void printMoves(Moves& moves);
std::string hashMove(std::pair<Tile, Tile> move);
std::string hashMove(std::unique_ptr<Move>& move);
bool matches(Moves& result, std::vector<std::pair<Tile, Tile>> expected);

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
                        std::array<int, N> bKing,
                        std::array<int, N> wKing,
                        std::array<bool, N> isWhiteKingInCheck,
                        std::array<bool, N> isBlackKingInCheck,
                        std::array<ull, N> whiteBB,
                        std::array<ull, N> blackBB);

template<std::size_t N>
void testEvaluator(std::function<Evaluation::Score(const Board&)> evaluatingFunction,
                    std::array<ChessBoard, N> testBoards,
                    std::array<std::optional<int>, N> enpassant,
                    std::array<std::vector<char>, N> castlingPrivileges,
                    std::array<bool, N> kingInCheckWhite,
                    std::array<bool, N> kingInCheckBlack,
                    std::array<ull, N> whiteAttackers,
                    std::array<ull, N> blackAttackers);

void testAttackGeneration();
void testKingMoveGeneration();
void testPawnMoveGeneration();
void testBishopMoveGeneration();
void testKnightMoveGeneration();
void testQueenMoveGeneration();
void testRookMoveGeneration();
void testEvaluation();

// Global variables
extern std::unordered_map<char, int> pieceIndex;

#endif // TESTS_HPP