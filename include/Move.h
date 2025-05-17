#ifndef MOVE_H
#define MOVE_H

#include "flags.h"
#include "pieces.h"

class Move {
  int start, end;
  int flg;
  Pieces::piece piece;

 public:
  Move(int start, int end, Pieces::piece p)
      : start(start), end(end), flg(Flags::NONE), piece(p) {};
  Move(int start, int end, Pieces::piece p, Flags::flag f)
      : start(start), end(end), flg(f), piece(p) {};

  int i() const { return start; }
  int f() const { return end; }

  Pieces::piece p() const { return piece; }

  int flag() const { return flg; }
  void setFlag(Flags::flag f) { flg |= static_cast<int>(f); }

  bool isCapture() const { return flg & static_cast<int>(Flags::CAPTURE); }
  bool isEnpassant() const { return flg & static_cast<int>(Flags::ENPASSANT); }
  bool isPromotion() const { return flg & static_cast<int>(Flags::PROMOTION); } 
  bool isCastle() const { return flg & static_cast<int>(Flags::CASTLE); }
  bool isDoublePawnPush() const { return flg & static_cast<int>(Flags::DOUBLESTEP); }
  bool isCheck() const { return flg & static_cast<int>(Flags::CHECK); }
};

#endif