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

const std::unordered_map<char, int> Board::castlingDestination = {
    {'k', 57},
    {'q', 61},
    {'K', 1},
    {'Q', 5}
};

void Board::setBit(ull& bb, int bit) { bb |= 1ULL << bit; }

void Board::clearBit(ull& bb, int bit) { bb &= ~(1ULL << bit); }

ull& Board::getBB(Pieces::piece piece) { return bbs[piece]; }

ull Board::readBB(Pieces::piece piece) const { return bbs[piece]; }

ull Board::readBB(Pieces::type pieceType, Side side) const { return bbs[pieceType + 6 * (side == WHITE)]; }

int Board::getEnpassantSquare() const { return enpassantSquare.has_value() ? enpassantSquare.value() : -1; }

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

void Board::updateCombinedBB(Move* move) {
  ull& currCombinedBB = getCombinedBB();
  clearBit(currCombinedBB, move->i());
  setBit(currCombinedBB, move->f());
}

void Board::updateWhiteBB(Move* move) {
  ull& currWhiteBB = getWhiteBB();
  clearBit(currWhiteBB, move->i());
  setBit(currWhiteBB, move->f());
}

void Board::updateBlackBB(Move* move) {
  ull& currBlackBB = getBlackBB();
  clearBit(currBlackBB, move->i());
  setBit(currBlackBB, move->f());
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
      blackBB{0xffff000000000000},
      knightAttackBitmaps{compileKnightAttacks()},
      kingAttackBitmaps{compileKingAttacks()},
      slidingAttackBitmaps{compileSlidingAttacks()} {}

Board::Board(ull* currBBs, std::vector<char> currCastlingPrivileges,
             std::optional<int> currEnpassantSquare, int currBKing,
             int currWKing)
    : castlingPrivileges{currCastlingPrivileges},
      enpassantSquare{currEnpassantSquare},
      bKing{currBKing}, 
      wKing{currWKing}, 
      combinedBB{findCombinedBB(currBBs)},
      whiteBB{findWhiteBB(currBBs)},
      blackBB{findBlackBB(currBBs)},
      knightAttackBitmaps{compileKnightAttacks()},
      kingAttackBitmaps{compileKingAttacks()},
      slidingAttackBitmaps{compileSlidingAttacks()} {
  std::copy(currBBs, currBBs + 12, bbs);
}

int Board::king(Side side) const { return side ? wKing : bKing; }

void Board::makeMove(Move* move) {
  if (move->flag() & Flags::ENPASSANT) {
    clearBit(getCombinedBB(), move->f() & 7 + move->i() & 56);
    clearBit(getBB(move->p()), move->f() & 7 + move->i() & 56);
  }
  clearBit(getBB(move->p()), move->i());
  setBit(getBB(move->p()), move->f());
  updateCombinedBB(move);

  if (move->flag() & Flags::CASTLE) {
    int isLeftCastle = move->f() < move->i();
    int oldRookSquare = isLeftCastle * 7 + (move->i() & 56); 
    int newRookSquare = (move->i() + move->f()) / 2;

    auto piece_t = getSide(move->p()) == WHITE ? Pieces::R : Pieces::r;
    clearBit(getCombinedBB(), oldRookSquare);
    clearBit(getBB(piece_t), oldRookSquare);
    setBit(getCombinedBB(), newRookSquare);
    setBit(getBB(piece_t), newRookSquare);
  }

  // update enpassant square
  if ((move->p() == Pieces::P || move->p() == Pieces::p) && (move->flag() & Flags::ENPASSANT)) {
    enpassantSquare = move->f();
  } else {
    enpassantSquare = std::nullopt;
  }

  // update castling privileges
  if (getSide(move->p()) == WHITE){
    updateWhiteBB(move);
    if (move->p() == Pieces::K) {
      stripCastlingPrivileges('K');
      stripCastlingPrivileges('Q');
    } else if (move->p() == Pieces::R) {
      (move->i() & 7) ? stripCastlingPrivileges('Q') : stripCastlingPrivileges('K');
    } 
  } else {
    updateBlackBB(move);
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
  if (move->flag() & Flags::CASTLE) {
    int isLeftCastle = move->f() < move->i();
    int oldRookSquare = isLeftCastle * 7 + (move->i() & 56); 
    int newRookSquare = (move->i() + move->f()) / 2;
    
    auto rook_t = getSide(move->p()) == WHITE ? Pieces::R : Pieces::r; 
    Move rookReverse(newRookSquare, oldRookSquare, rook_t);
    makeMove(&rookReverse);

    // reset castling privileges
    auto king = getSide(move->p()) == WHITE ? 'K' : 'k';
    auto queen = getSide(move->p()) == WHITE ? 'Q' : 'q';
    castlingPrivileges.push_back(king);
    if (isLeftCastle) {
      castlingPrivileges.push_back(queen);
    }
  } else if (move->flag() & Flags::ENPASSANT) {
    int enpassantSquare = move->f() & 7 + move->i() & 56;
    // insert pawn at enpassant square onto board
    auto pawn_t = getSide(move->p()) == WHITE ? Pieces::P : Pieces::p;
    setBit(getCombinedBB(), enpassantSquare);
    setBit(getBB(pawn_t), enpassantSquare);
    setBit(getSide(move->p()) == WHITE ? whiteBB : blackBB, enpassantSquare);

    Move enpassantReverse(move->f(), move->i(), move->p());
    makeMove(&enpassantReverse);
  } else if (move->flag() & Flags::PROMOTION) {
    auto pawn_t = getSide(move->p()) == WHITE ? Pieces::P : Pieces::p;
    setBit(getBB(pawn_t), move->i());
    setBit(getCombinedBB(), move->i());
    setBit(getSide(move->p()) == WHITE ? whiteBB : blackBB, move->i());

    // ASSUME PROMOTION TO QUEEN
    auto queen_t = getSide(move->p()) == WHITE ? Pieces::Q : Pieces::q;
    clearBit(getCombinedBB(), move->f()); 
    clearBit(getBB(queen_t), move->f());
    clearBit(getSide(move->p()) == WHITE ? whiteBB : blackBB, move->f());
  } else {
    Move reverse(move->f(), move->i(), move->p());
    makeMove(&reverse);
  }
}

ull Board::readCombinedBB() const { return combinedBB; }

ull Board::readWhiteBB() const { return whiteBB; }

ull Board::readBlackBB() const { return blackBB; }

ull Board::allyBB(Side side) const { return side ? whiteBB : blackBB; }

ull Board::opposingBB(Side side) const { return side ? blackBB : whiteBB; }

std::array<ull, 64> Board::knightAttacks() const { return knightAttackBitmaps; }

std::array<ull, 64> Board::kingAttacks() const { return kingAttackBitmaps; }

std::array<std::array<ull, 8>, 64> Board::slidingAttacks() const { return slidingAttackBitmaps; }

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
        stringifiedBoard[7 - (i / 8)][i % 8] = it->first;
      }
    }
  }
  for (const auto& row : stringifiedBoard) {
    for (char c : row) std::cout << c;
    std::cout << '\n';
  }
}
