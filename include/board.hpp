#ifndef BOARD_H
#define BOARD_H

#include <algorithm>
#include <iostream>
#include <vector>
#include <optional>
#include <unordered_map>

#include "Move.h"
#include "attacks.h"
#include "pieces.h"
#include "sides.h"

typedef unsigned long long ull;

class Board {
  ull bbs[12];
  std::vector<char> castlingPrivileges;
  std::optional<int> enpassantSquare;
  int bKing, wKing;
  static const std::unordered_map<char, int> castlingDestination;

  ull combinedBB, whiteBB, blackBB;

  std::array<ull, 64> knightAttackBitmaps, kingAttackBitmaps;
  std::array<std::array<ull, 8>, 64> slidingAttackBitmaps;

  void setBit(ull&, int);
  void clearBit(ull&, int);

  ull& getBB(Pieces::piece);

  // change name of each findXX to computeXX
  ull findCombinedBB(ull*);
  ull findWhiteBB(ull*);
  ull findBlackBB(ull*);

  ull& getCombinedBB();
  ull& getWhiteBB();
  ull& getBlackBB();

  void updateCombinedBB(Move*);
  void updateWhiteBB(Move*);
  void updateBlackBB(Move*);

  inline void stripCastlingPrivileges(char);

 public:
  // simplify second constructor 
  Board();
  Board(ull*, std::vector<char>, std::optional<int>, int, int);

  int king(Side) const;
  bool isKingInCheck(Side) const;

  void makeMove(Move*);
  void undoMove(Move*);

  ull getAttackers(Side) const;
  void setAttackers(Side, int);
  void clearAttackers(Side);

  int getEnpassantSquare() const;
  std::vector<char> getCastlingPrivileges() const;
  std::vector<int> availableCastlingDestinations(Side) const;

  ull readBB(Pieces::piece) const;
  ull readBB(Pieces::type, Side) const;

  ull readCombinedBB() const;
  ull readWhiteBB() const;
  ull readBlackBB() const;

  ull allyBB(Side) const;
  ull opposingBB(Side) const;

  std::array<ull, 64> knightAttacks() const;
  std::array<ull, 64> kingAttacks() const;
  std::array<std::array<ull, 8>, 64> slidingAttacks() const;

  void printBoard() const;
};

namespace Attack { // forward declaration from kingSafety.cpp
  // bool doesMovePutKingInCheck(Board*, Move*, Side);
}

#endif