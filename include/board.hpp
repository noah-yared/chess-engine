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

static inline const std::unordered_map<char, int> castlingDestination = {
    {'k', 57},
    {'q', 61},
    {'K', 1},
    {'Q', 5}
};

namespace BoardEncoding {
enum BIT_OFFSETS {
    CASTLING_PRIVS = 0,
    WKING = 4,
    BKING = 10,
    EXISTS_ENPASSANT_SQ = 16,
    ENPASSANT_SQ = 17
};

inline std::unordered_map<char, int> PRIV_BIT_OFFSET = {
  {'k', 0}, {'q', 1}, {'K', 2}, {'Q', 3}
};
};

class Board {
  ull bbs[12];
  uint32_t bitfield; /* castlingPrivs + wKing + bKing + existsEnpassantSq + enpassantSq (if exists) */
  ull whiteBB, blackBB; /* combinedBB = whiteBB | blackBB */

  void revertState(uint32_t oldState) { bitfield = oldState; }

  int castlingPrivs() const { return (bitfield >> BoardEncoding::BIT_OFFSETS::CASTLING_PRIVS) & 0xf; }
  // int wKing() const { return (bitfield >> BoardEncoding::BIT_OFFSETS::WKING) & 0x3f; }
  int wKing() const { return __builtin_ctzll(readBB(Pieces::piece::K)); }
  // int bKing() const { return (bitfield >> BoardEncoding::BIT_OFFSETS::BKING) & 0x3f; }
  int bKing() const { return __builtin_ctzll(readBB(Pieces::piece::k)); }
  bool existsEnpassantSq() const { return (bitfield >> BoardEncoding::BIT_OFFSETS::EXISTS_ENPASSANT_SQ) & 0x1; }
  int enpassantSq() const { return (bitfield >> BoardEncoding::BIT_OFFSETS::ENPASSANT_SQ) & 0x3f; }

  void setWhiteKing(int square) { bitfield |= (square << BoardEncoding::BIT_OFFSETS::WKING); }
  void setBlackKing(int square) { bitfield |= (square << BoardEncoding::BIT_OFFSETS::BKING); }

  void setBit(ull&, int);
  void clearBit(ull&, int);

  void removePiece(Pieces::piece, int);
  void placePiece(Pieces::piece, int);

  ull findWhiteBB(ull*);
  ull findBlackBB(ull*);

  ull& getBB(Pieces::piece);
  ull& getWhiteBB();
  ull& getBlackBB();

  inline void stripCastlingPrivileges(char);

 public:
  Board();
  Board(ull*, std::vector<char>, std::optional<int>, int, int);
  Board(ull*, uint32_t);

  uint32_t pullState() const { return bitfield; }

  int king(::Side) const;

  void makeMove(Move*);
  void undoMove(Move*, uint32_t);

  std::optional<int> getEnpassantSquare() const;
  void setEnpassantSquare(std::optional<int>);

  std::vector<int> availableCastlingDestinations(::Side) const;

  ull readBB(Pieces::piece) const;
  ull readBB(Pieces::type, ::Side) const;

  ull readCombinedBB() const;
  ull readWhiteBB() const;
  ull readBlackBB() const;

  ull allyBB(::Side) const;
  ull opposingBB(::Side) const;

  inline std::array<ull, 64> knightAttacks() const { return knightAttackBitmaps; };
  inline std::array<ull, 64> kingAttacks() const { return kingAttackBitmaps; };
  inline std::array<std::array<ull, 8>, 64> slidingAttacks() const { return slidingAttackBitmaps; };

  void printBoard() const;
};

#endif