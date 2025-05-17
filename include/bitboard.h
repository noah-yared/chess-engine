#ifndef BITBOARD_H
#define BITBOARD_H

#include <cstdint>
#include <unordered_map>
#include <vector>

enum Piece {
  WHITEPAWN,
  WHITEROOK,
  WHITEKNIGHT,
  WHITEBISHOP,
  WHITEQUEEN,
  WHITEKING,
  BLACKPAWN,
  BLACKROOK,
  BLACKKNIGHT,
  BLACKBISHOP,
  BLACKQUEEN,
  BLACKKING
};

Piece whitePieces[6]{WHITEPAWN, WHITEROOK, WHITEKNIGHT, WHITEBISHOP,
                     WHITEQUEEN, WHITEKING};
Piece blackPieces[6]{BLACKPAWN, BLACKROOK, BLACKKNIGHT, BLACKBISHOP,
                     BLACKQUEEN, BLACKKING};

std::unordered_map<char, int> pieceMap = {
    {'p', BLACKPAWN},   {'r', BLACKROOK},  {'n', BLACKKNIGHT},
    {'b', BLACKBISHOP}, {'q', BLACKQUEEN}, {'k', BLACKKING},
    {'P', WHITEPAWN},   {'R', WHITEROOK},  {'N', WHITEKNIGHT},
    {'B', WHITEBISHOP}, {'Q', WHITEQUEEN}, {'K', WHITEKING}};

#endif