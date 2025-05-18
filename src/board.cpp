#include "board.hpp"

#include <array>
#include <cctype>
#include <algorithm>
#include <vector>
#include <unordered_map>

#include "attacks.h"
#include "pieces.h"
#include "sides.h"

using ull = uint64_t;

void Board::setBit(ull& bb, int bit) { bb |= 1ULL << bit; }

void Board::clearBit(ull& bb, int bit) { bb &= ~(1ULL << bit); }

void Board::removePiece(Pieces::piece piece, int square) {
  clearBit(getBB(piece), square);
  clearBit(getSide(piece) == WHITE ? whiteBB : blackBB, square);
  clearBit(combinedBB, square);
}

void Board::placePiece(Pieces::piece piece, int square) {
  setBit(getBB(piece), square);
  setBit(getSide(piece) == WHITE ? whiteBB : blackBB, square);
  setBit(combinedBB, square);
}

ull& Board::getBB(Pieces::piece piece) { return bbs[piece]; }

ull Board::readBB(Pieces::piece piece) const { return bbs[piece]; }

ull Board::readBB(Pieces::type pieceType, Side side) const { return bbs[pieceType + 6 * (side == WHITE)]; }

int Board::getEnpassantSquare() const { return enpassantSquare.has_value() ? enpassantSquare.value() : -1; }

void Board::setEnpassantSquare(std::optional<int> square) { enpassantSquare = square; }

std::vector<char> Board::getCastlingPrivileges() const { return castlingPrivileges; }

std::vector<int> Board::availableCastlingDestinations(Side side) const {
  std::vector<int> destinations; 
  for (char priv : castlingPrivileges) {
    if (std::isupper(priv) != (side == WHITE)) continue;
    destinations.push_back(castlingDestination.at(priv)); 
  }
  return destinations;
}

ull& Board::getCombinedBB() { return combinedBB; }

ull& Board::getWhiteBB() { return whiteBB; }

ull& Board::getBlackBB() { return blackBB; }

ull Board::findCombinedBB(ull* bbs) {
  ull combinedBB = 0ULL;
  for (int i = 0; i < 12; i++) {
    combinedBB |= *bbs++;
  }
  return combinedBB;
}

ull Board::findWhiteBB(ull* bbs) {
  ull whiteBB = 0ULL;
  bbs += 6;
  for (int i = 0; i < 6; i++)
    whiteBB |= *bbs++;
  return whiteBB;
}

ull Board::findBlackBB(ull* bbs) {
  ull blackBB = 0ULL;
  for (int i = 0; i < 6; i++)
    blackBB |= *bbs++;
  return blackBB;
}

void Board::stripCastlingPrivileges(char priv) {
  auto it = std::remove(castlingPrivileges.begin(), castlingPrivileges.end(), priv);
  castlingPrivileges.erase(it, castlingPrivileges.end());
}

Board::Board()
    : bbs{0xff000000000000ULL,
          0x8100000000000000ULL,
          0x4200000000000000ULL,
          0x2400000000000000ULL,
          0x1000000000000000ULL,
          0x800000000000000ULL,
          0xff00ULL,
          0x81ULL,
          0x42ULL,
          0x24ULL,
          0x10ULL,
          0x8ULL},
      castlingPrivileges{'k', 'q', 'K', 'Q'},
      enpassantSquare{std::nullopt},
      bKing{59},
      wKing{3},
      combinedBB{0xffff00000000ffff},
      whiteBB{0xffff},
      blackBB{0xffff000000000000} {}

Board::Board(ull* currBBs, std::vector<char> currCastlingPrivileges,
             std::optional<int> currEnpassantSquare, int currBKing,
             int currWKing)
    : castlingPrivileges{currCastlingPrivileges},
      enpassantSquare{currEnpassantSquare},
      bKing{currBKing}, 
      wKing{currWKing}, 
      combinedBB{findCombinedBB(currBBs)},
      whiteBB{findWhiteBB(currBBs)},
      blackBB{findBlackBB(currBBs)} {
  std::copy(currBBs, currBBs + 12, bbs);
}

int Board::king(Side side) const { return side ? wKing : bKing; }

void Board::makeMove(Move* move) {
  removePiece(move->p(), move->i());
  placePiece(move->p(), move->f());

  // udpate king positions
  if (move->p() == Pieces::piece::K) {
    wKing = move->f(); // update white king position
  } else if (move->p() == Pieces::piece::k) {
    bKing = move->f(); // update black king position
  }

  // handle pawn double step 
  if (move->isDoublePawnPush()) {
    setEnpassantSquare(std::optional<int>((move->i() + move->f()) / 2));
  } else {
    setEnpassantSquare(std::nullopt);
  }

  // handle capture
  if (move->isCapture()) {
    removePiece(move->capturedPiece(), move->f());
  }

  // handle enpassant
  if (move->isEnpassant()) {
    // removed captured pawn
    removePiece(getSide(move->p()) == WHITE ? Pieces::p : Pieces::P, move->f() & 7 + move->i() & 56);
  }
  
  // handle castle
  if (move->isCastle()) {
    int isLeftCastle = move->f() < move->i();
    int oldRookSquare = isLeftCastle * 7 + (move->i() & 56); 
    int newRookSquare = (move->i() + move->f()) / 2;

    auto rook_t = getSide(move->p()) == WHITE ? Pieces::R : Pieces::r;
    removePiece(rook_t, oldRookSquare);
    placePiece(rook_t, newRookSquare);
  }

  // update enpassant square
  if ((move->p() == Pieces::P || move->p() == Pieces::p) && (move->isDoublePawnPush())) {
    enpassantSquare = move->f();
  } else {
    enpassantSquare = std::nullopt;
  }

  // handle promotion
  if (move->isPromotion()) {
    removePiece(move->p(), move->f());
    // AUTOMATIC PROMOTION TO QUEEN
    placePiece(getSide(move->p()) == WHITE ? Pieces::Q : Pieces::q, move->f());
  }

  // update castling privileges
  if (getSide(move->p()) == WHITE){
    // updateWhiteBB(move);
    if (move->p() == Pieces::K) {
      stripCastlingPrivileges('K');
      stripCastlingPrivileges('Q');
    } else if (move->p() == Pieces::R) {
      (move->i() & 7) ? stripCastlingPrivileges('Q') : stripCastlingPrivileges('K');
    } 
  } else {
    // updateBlackBB(move);
    if (move->p() == Pieces::k) {
      stripCastlingPrivileges('k');
      stripCastlingPrivileges('q');
    } else if (move->p() == Pieces::r) {
      (move->i() & 7) ? stripCastlingPrivileges('q') : stripCastlingPrivileges('k');
    } 
  }
}

void Board::undoMove(Move* move) {
  if (!move) {
    std::cerr << "no move to undo";
  }

  if (!move->isPromotion()) {
    Move reverseMove(move->f(), move->i(), move->p());
    makeMove(&reverseMove);
  }

  if (move->isCapture()) {
    placePiece(move->capturedPiece(), move->f());
  }

  if (move->isDoublePawnPush()) {
    // remove enpassant square
    setEnpassantSquare(std::nullopt);
  }

  if (move->isCastle()) {
    int isLeftCastle = move->f() < move->i();
    int oldRookSquare = isLeftCastle * 7 + (move->i() & 56); 
    int newRookSquare = (move->i() + move->f()) / 2;
    auto rook_t = getSide(move->p()) == WHITE ? Pieces::R : Pieces::r; 
    // move rook back
    Move rookReverse(newRookSquare, oldRookSquare, rook_t);
    makeMove(&rookReverse);
    // reset castling privileges
    auto king = getSide(move->p()) == WHITE ? 'K' : 'k';
    auto queen = getSide(move->p()) == WHITE ? 'Q' : 'q';
    castlingPrivileges.push_back(king);
    if (isLeftCastle) {
      castlingPrivileges.push_back(queen);
    }
  } 
  
  if (move->isEnpassant()) {
    int enpassantSquare = move->f() & 7 + move->i() & 56;
    // insert captured pawn at enpassant square onto board
    auto pawn_t = getSide(move->p()) == WHITE ? Pieces::p : Pieces::P;
    placePiece(pawn_t, enpassantSquare);
    // set enpassant square
    setEnpassantSquare(enpassantSquare);
  } 
  
  if (move->isPromotion()) {
    // move promoted pawn back
    auto pawn_t = getSide(move->p()) == WHITE ? Pieces::P : Pieces::p;
    placePiece(pawn_t, move->i());
    // ASSUME PROMOTION TO QUEEN so remove queen
    auto queen_t = getSide(move->p()) == WHITE ? Pieces::Q : Pieces::q;
    removePiece(queen_t, move->f());
  }
}

ull Board::readCombinedBB() const { return combinedBB; }

ull Board::readWhiteBB() const { return whiteBB; }

ull Board::readBlackBB() const { return blackBB; }

ull Board::allyBB(Side side) const { return side ? whiteBB : blackBB; }

ull Board::opposingBB(Side side) const { return side ? blackBB : whiteBB; }

void Board::printBoard() const {
  std::unordered_map<char, int> pieceIndex { /* map pieces to bb index*/
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
    std::string bitstring = std::bitset<64>(readBB(static_cast<Pieces::piece>(it->second))).to_string();
    for (int i = 0; i < 64; i++) {
      if (bitstring[i] == '1') {
        stringifiedBoard[i / 8][i % 8] = it->first;
      }
    }
  }
  for (const auto& row : stringifiedBoard) {
    for (char c : row) std::cout << c;
    std::cout << '\n';
  }
}
