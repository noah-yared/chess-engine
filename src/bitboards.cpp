#include <bitset>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "bitboards.h"

// Complex constructors
Bitboards::Bitboards(const std::string& sBoard, FromAsciiBoard) noexcept : bbs_{}, whiteBB_{}, blackBB_{} {
  const std::string pieceString = "prnbqkPRNBQK";
  int bit = 63;
  for (char c : sBoard) {
    if (iswspace(c))
      continue;
    if (auto pos = pieceString.find(c); pos != std::string::npos)
      togglePieceSquare(static_cast<int>(pos), bit);
    --bit;
  }
  assert(bit == -1 && "Invalid ascii board format passed in! Make sure to specify all squares on the board!");
}

Bitboards::Bitboards(const std::string& fen, FromFEN) noexcept : bbs_{}, whiteBB_{}, blackBB_{} {
  const std::string pieceString = "prnbqkPRNBQK";
  const std::string fenPieces = fen.substr(0, fen.find_first_of(' '));
  int bit = 63;
  for (auto c : fenPieces) {
    if (c == '/') continue;
    if (isdigit(c)) bit -= (c - '0');
    else togglePieceSquare(static_cast<int>(pieceString.find(c)), bit--);
  }
}

// Complex methods
bool Bitboards::isConsistent() const noexcept {
  const std::string pieceString = "prnbqkPRNBQK";
  if (whiteBB_ & blackBB_) return false;
  if (combine(wStart(), wEnd()) != whiteBB_) return false;
  if (combine(bStart(), bEnd()) != blackBB_) return false;
  for (int i = 0; i < NUM_BITBOARDS; ++i) {
    for (int j = i + 1; j < NUM_BITBOARDS; ++j) {
      if (bbs_[i] & bbs_[j]) {
        std::cout << pieceString[i] << "'s bitboard and " << pieceString[j] << "'s bitboard overlap!" << std::endl;
        return false;
      }
    }
  }
  return true;
}

std::string Bitboards::parsePiecePlacement() const noexcept {
  std::stringstream ss;
  const std::string pieceStr = "prnbqkPRNBQK";
  for (int bit = 63, consecutiveEmpty = 0; bit; --bit) {
    auto pType = getPieceType(bit);
    if (pType == PieceType::NONE) {
      ++consecutiveEmpty;
      if (bit % FILES == 0) {
        ss << consecutiveEmpty << (bit ? "/" : "");
        consecutiveEmpty = 0;
      }
    } else {
      if (consecutiveEmpty)
        ss << consecutiveEmpty;
      ss << pieceStr[pieceToKey(pType, getPieceColor(bit))];
      if (bit && bit % FILES == 0)
        ss << '/';
      consecutiveEmpty = 0;
    }
  }
  return ss.str();
}

std::string Bitboards::toString() const noexcept {
  const std::string pieceString = "prnbqkPRNBQK";
  std::stringstream ss;
  for (int r = 7; r >= 0; --r) {
    ss << "    "; // indent each rank
    for (int c = 7; c >= 0; --c) {
      int sq = r * FILES + c;
      if (auto pType = getPieceType(sq); pType != PieceType::NONE) {
        assert(pType <= PieceType::KING && "Invalid piece type!");
        bool isWhite = whiteBB() & (1ULL << sq);
        ss << pieceString[pieceToKey(pType, isWhite ? Color::WHITE : Color::BLACK)];
      } else {
        ss << '.';
      }
    }
    ss << '\n';
  }
  return ss.str();
} 

std::ostream& operator<<(std::ostream& os, const Bitboards& bitboards) noexcept {
  const std::string pieceString = "prnbqkPRNBQK";
  os   << "Bitboards(\nbbs_=" 
       << "[\n";
  for (int i = 0; i < NUM_BITBOARDS; ++i) {
    os << "       " << pieceString[i] << ": " << std::bitset<64>(bitboards.bbs_[i]) << ",\n";
  }
  os   << "]\n"
       << "whiteBB_: " << std::bitset<64>(bitboards.whiteBB_) << "\n"
       << "blackBB_: " << std::bitset<64>(bitboards.blackBB_) << "\n)";
  return os;
};
