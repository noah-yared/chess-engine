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

static uint32_t computeBitfield(std::vector<char> castlingPrivs, std::optional<int> enpassantSq, int bKing, int wKing) {
  uint32_t bitfield = 0;  
  for (char priv : castlingPrivs) {
    bitfield |= 1 << (BoardEncoding::PRIV_BIT_OFFSET[priv] + BoardEncoding::BIT_OFFSETS::CASTLING_PRIVS);
  }
  bitfield |= wKing << BoardEncoding::BIT_OFFSETS::WKING;
  bitfield |= bKing << BoardEncoding::BIT_OFFSETS::BKING;
  if (enpassantSq.has_value()) {
    bitfield |= 1 << BoardEncoding::BIT_OFFSETS::EXISTS_ENPASSANT_SQ;
    bitfield |= enpassantSq.value() << BoardEncoding::BIT_OFFSETS::ENPASSANT_SQ;
  }
  return bitfield;
}

void Board::setBit(ull& bb, int bit) { bb |= 1ULL << bit; }
void Board::clearBit(ull& bb, int bit) { bb &= ~(1ULL << bit); }

void Board::removePiece(Pieces::piece piece, int square) {
  clearBit(getBB(piece), square);
  clearBit(getSide(piece) == WHITE ? whiteBB : blackBB, square);
}
void Board::placePiece(Pieces::piece piece, int square) {
  setBit(getBB(piece), square);
  setBit(getSide(piece) == WHITE ? whiteBB : blackBB, square);
}

ull& Board::getBB(Pieces::piece piece) { return bbs[piece-1]; }
ull Board::readBB(Pieces::piece piece) const { return bbs[piece-1]; }
ull Board::readBB(Pieces::type pieceType, Side side) const { return bbs[pieceType + 6 * (side == WHITE)]; }

std::optional<int> Board::getEnpassantSquare() const { return existsEnpassantSq() ? std::optional<int>(enpassantSq()) : std::nullopt; }
void Board::setEnpassantSquare(std::optional<int> square) { 
  if (square.has_value()) {
    bitfield |= (1 << BoardEncoding::BIT_OFFSETS::EXISTS_ENPASSANT_SQ);
    bitfield |= (square.value() << BoardEncoding::BIT_OFFSETS::ENPASSANT_SQ);
  } else {
    bitfield &= ~(1 << BoardEncoding::BIT_OFFSETS::EXISTS_ENPASSANT_SQ);
    bitfield &= ~(0x3f << BoardEncoding::BIT_OFFSETS::ENPASSANT_SQ); // clear enpassant square
  }
}

std::vector<int> Board::availableCastlingDestinations(Side side) const {
  int currentPrivs = castlingPrivs();
  if (side == WHITE) currentPrivs >>= 2;
  std::vector<int> destinations;
  if (currentPrivs & 1)  destinations.push_back(castlingDestination.at(side == WHITE ? 'K' : 'k'));
  if (currentPrivs & 2)  destinations.push_back(castlingDestination.at(side == WHITE ? 'Q' : 'q'));
  return destinations;
}

ull& Board::getWhiteBB() { return whiteBB; }
ull& Board::getBlackBB() { return blackBB; }

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
  bitfield &= ~(1 << (BoardEncoding::PRIV_BIT_OFFSET[priv] + BoardEncoding::BIT_OFFSETS::CASTLING_PRIVS));
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
      bitfield{0b000000'0'111011'000011'1111},
      whiteBB{0xffff},
      blackBB{0xffff000000000000} {}

Board::Board(ull* currBBs, std::vector<char> currCastlingPrivileges,
             std::optional<int> currEnpassantSquare, int currBKing,
             int currWKing)
    : bitfield{computeBitfield(currCastlingPrivileges, currEnpassantSquare, currBKing, currWKing)},
      whiteBB{findWhiteBB(currBBs)},
      blackBB{findBlackBB(currBBs)} {
  std::copy(currBBs, currBBs + 12, bbs);
}

Board::Board(ull* currBBs, uint32_t bitfield) : bitfield{bitfield}, whiteBB{findWhiteBB(currBBs)}, blackBB{findBlackBB(currBBs)} {
  std::copy(currBBs, currBBs + 12, bbs);
}

int Board::king(Side side) const { return side == WHITE ? wKing() : bKing(); }

void Board::makeMove(Move* move) {
  removePiece(move->p(), move->i());
  placePiece(move->p(), move->f());

  // udpate king positions
  if (move->p() == Pieces::piece::K) {
    setWhiteKing(move->f()); // update white king position
  } else if (move->p() == Pieces::piece::k) {
    setBlackKing(move->f()); // update black king position
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
    int isLeftCastle = move->f() > move->i();
    int oldRookSquare = isLeftCastle * 7 + (move->i() & 56); 
    int newRookSquare = (move->i() + move->f()) / 2;

    auto rook_t = getSide(move->p()) == WHITE ? Pieces::R : Pieces::r;
    removePiece(rook_t, oldRookSquare);
    placePiece(rook_t, newRookSquare);
  }

  // update enpassant square
  if ((move->p() == Pieces::P || move->p() == Pieces::p) && (move->isDoublePawnPush())) {
    setEnpassantSquare(move->f());
  } else {
    setEnpassantSquare(std::nullopt);
  }

  // handle promotion
  if (move->isPromotion()) {
    removePiece(move->p(), move->f());
    // AUTOMATIC PROMOTION TO QUEEN
    placePiece(getSide(move->p()) == WHITE ? Pieces::Q : Pieces::q, move->f());
  }

  // update castling privileges
  if (getSide(move->p()) == WHITE){
    if (move->p() == Pieces::K) {
      stripCastlingPrivileges('K');
      stripCastlingPrivileges('Q');
    } else if (move->p() == Pieces::R) {
      (move->i() & 7) ? stripCastlingPrivileges('Q') : stripCastlingPrivileges('K');
    } 
  } else {
    if (move->p() == Pieces::k) {
      stripCastlingPrivileges('k');
      stripCastlingPrivileges('q');
    } else if (move->p() == Pieces::r) {
      (move->i() & 7) ? stripCastlingPrivileges('q') : stripCastlingPrivileges('k');
    } 
  }
}

void Board::undoMove(Move* move, uint32_t parentState) {
  // revert non-bb state
  revertState(parentState);

  // revert bb state
  if (move->isPromotion()) {
    // move promoted pawn back
    auto pawn_t = getSide(move->p()) == WHITE ? Pieces::P : Pieces::p;
    placePiece(pawn_t, move->i());
    // ASSUME PROMOTION TO QUEEN so remove queen
    auto queen_t = getSide(move->p()) == WHITE ? Pieces::Q : Pieces::q;
    removePiece(queen_t, move->f());
  } else {
    placePiece(move->p(), move->i());
    removePiece(move->p(), move->f());

    // revert to pre-capture
    if (move->isCapture()) {
      placePiece(move->capturedPiece(), move->f());
    }

    // revert to pre-castle
    if (move->isCastle()) {
      int isLeftCastle = move->f() > move->i();
      int oldRookSquare = isLeftCastle * 7 + (move->i() & 56); 
      int newRookSquare = (move->i() + move->f()) / 2;
      auto rook_t = getSide(move->p()) == WHITE ? Pieces::R : Pieces::r; 
      // move rook back
      removePiece(rook_t, newRookSquare);
      placePiece(rook_t, oldRookSquare);
    } 

    // revert to pre-enpassant
    if (move->isEnpassant()) {
      int enpassantSquare = move->f() & 7 + move->i() & 56;
      // insert captured pawn at enpassant square onto board
      auto pawn_t = getSide(move->p()) == WHITE ? Pieces::p : Pieces::P;
      placePiece(pawn_t, enpassantSquare);
    } 
  }
}

ull Board::readCombinedBB() const { return whiteBB | blackBB; }
ull Board::readWhiteBB() const { return whiteBB; }
ull Board::readBlackBB() const { return blackBB; }

ull Board::allyBB(Side side) const { return side ? whiteBB : blackBB; }
ull Board::opposingBB(Side side) const { return side ? blackBB : whiteBB; }

void Board::printBoard() const {
  std::unordered_map<char, int> pieceIndex { /* map pieces to bb index*/
    {'p', 1}, {'P', 7},
    {'r', 2}, {'R', 8},
    {'n', 3}, {'N', 9},
    {'b', 4}, {'B', 10},
    {'q', 5}, {'Q', 11},
    {'k', 6}, {'K', 12}
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
