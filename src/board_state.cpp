#include <sstream>
#include <string>

#include "board_state.h"
#include "board_utils.h"

// Complex constructors
BoardState::BoardState(const std::string& turn, const std::string& castlingRights, const std::string& enpassant) noexcept : state_{} {
  setTurn(turn);
  setCastlingPrivileges(castlingRights);
  setEnpassantSquare(enpassant == "-"
      ? std::nullopt
      : std::optional{ algebraicNotationToIndex(enpassant) }
  );
}

BoardState::BoardState(const std::string& fen) noexcept {
  size_t start = fen.find_first_of(' ') + 1;
  setTurn(fen.substr(start, 1));
  start += 2;
  setCastlingPrivileges(fen.substr(start, fen.find_first_of(' ', start) - start));
  start = fen.find_first_of(' ', start) + 1;
  setEnpassantSquare((fen.find_first_of(' ', start) - start != 2)
      ? std::nullopt
      : std::optional{ algebraicNotationToIndex(fen.substr(start, 2)) }
  );
}

// Private helper methods
void BoardState::setTurn(const std::string& color) noexcept {
  if ((color == "b") != blackToMove()) updateTurn(); // toggle turn due to mismatch
}

void BoardState::setCastlingPrivileges(const std::string& privs) noexcept {
  u32 rights = 0;
  if (privs != "-")
    for (char c : privs)
      rights |= 1 << PRIV_BIT_OFFSET(c);
  state_ &= ~(0xf << CASTLING_PRIVS); // clear
  state_ |= rights << CASTLING_PRIVS; // set
}

// Complex logic methods
std::vector<int> BoardState::availableCastlingDestinations(Color color) const noexcept {
  // white castling bits are the upper 2 bits of the castling bits
  std::vector<int> dests; dests.reserve(2); 
  int isWhite = color == Color::WHITE, isBlack = color == Color::BLACK;
  char kingside = 'k' * isBlack + 'K' * isWhite, queenside = 'q' * isBlack + 'Q' * isWhite;
  if (isCastlingRightAvailable(kingside))
    dests.push_back(CASTLING_DESTINATION(kingside));
  if (isCastlingRightAvailable(queenside))
    dests.push_back(CASTLING_DESTINATION(queenside));
  return dests;
}

// String parsing methods
std::string BoardState::parseCastlingRights() const noexcept {
  if (!castlingBits())
    return "-";
  std::stringstream ss;
  for (char right : { 'K', 'Q', 'k', 'q' })
    if (isCastlingRightAvailable(right))
      ss << right;
  return ss.str();
}

std::string BoardState::parseTurn() const noexcept {
  return blackToMove() ? "b" : "w";
}

std::string BoardState::parseEnpassantSquare() const noexcept {
  if (!existsEnpassantSq())
    return "-";
  return indexToAlgebraicNotation(enpassantSq());
}

std::string BoardState::toString() const noexcept {
  return parseCastlingRights() + " " + parseTurn() + " " + parseEnpassantSquare();
}

// Complex comparison operators
bool BoardState::operator==(const BoardState& other) const noexcept {
  return castlingBits() == other.castlingBits() && getTurn() == other.getTurn()
      && getEnpassantSquare().value_or(0) == other.getEnpassantSquare().value_or(0);
}

bool BoardState::operator!=(const BoardState& other) const noexcept { 
  return !(operator==(other)); 
} 
