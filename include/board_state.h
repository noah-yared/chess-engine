#pragma once

#include <cassert>
#include <concepts>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "board_state_snapshot.h"
#include "pieces.h"
#include "platform.h"
#include "traits.h"

class BoardState {
  u32 state_; // castlingPrivs + existsEnpassantSq + enpassantSq (junk if existsEnpassantSq is 0) + turn (0 -> white to move, 1 -> black to move)

  enum BIT_OFFSETS {
    CASTLING_PRIVS = 0, // 4 bits, corresponding to kqKQ respectively (1 = castle available, 0 = castle unavailable)
    TURN = 4, // 1 = black to move, 0 = white to move
    EXISTS_ENPASSANT_SQ = 5, // 1 = enpassant square exists, 0 = enpassant square does not exist
    ENPASSANT_SQ = 6, // 6 bits, enpassant square if existsEnpassantSq is 1
  };

  static constexpr u32 encode(const std::vector<char>& castlingRights, Color playerTurn, std::optional<int> enpassantSq) {
    u32 state = 0;
    for (char c : castlingRights) {
      state |= 1 << (BIT_OFFSETS::CASTLING_PRIVS + PRIV_BIT_OFFSET(c));
    }
    state |= (playerTurn == Color::BLACK) << BIT_OFFSETS::TURN;
    state |= (enpassantSq.has_value()) << BIT_OFFSETS::EXISTS_ENPASSANT_SQ;
    state |= enpassantSq.value_or(0) << BIT_OFFSETS::ENPASSANT_SQ;
    return state;
  }

  [[nodiscard]] bool existsEnpassantSq() const { return (state_ >> BIT_OFFSETS::EXISTS_ENPASSANT_SQ) & 0x1; }
  [[nodiscard]] int enpassantSq() const { return (state_ >> BIT_OFFSETS::ENPASSANT_SQ) & 0x3f; }

  void setTurn(std::string color) {
    if ((color == "b") != blackToMove()) // need to toggle turn due to mismatch
      updateTurn();
  }

  void setCastlingPrivileges(std::string privs) {
    u32 rights = 0;
    if (privs == "-")
      privs = "";
    for (char c : privs)
      rights |= 1 << PRIV_BIT_OFFSET(c);
    state_ &= ~(0xf << CASTLING_PRIVS); // clear
    state_ |= rights << CASTLING_PRIVS; // set
  }

  static int algebraicNotationToIndex(std::string algebraicNotation) {
    assert(algebraicNotation.size() == 2);
    return 7 - (algebraicNotation[0] - 'a') + ((algebraicNotation[1] - '1') << 3);
  }

  static std::string indexToAlgebraicNotation(int index) {
    std::cout << "converting " << index << " to algebraic notation...";
    std::stringstream ss;
    char rank = '1' + (index / FILES);
    char file = 'a' + 7 - (index % RANKS);
    ss << file << rank;
    return ss.str();
  }

public:
  BoardState() : state_{encode({'K', 'Q', 'k', 'q'}, Color::WHITE, std::nullopt)} {};
  explicit BoardState(u32 state) : state_{state} {};
  explicit BoardState(const BoardStateSnapshot& snapshot) : state_{snapshot.state} {};
  BoardState(std::string turn, std::string castlingRights, std::string enpassant) : state_{} {
    setTurn(turn);
    setCastlingPrivileges(castlingRights);
    setEnpassantSquare(enpassant == "-"
        ? std::nullopt
        : std::optional{ algebraicNotationToIndex(enpassant) }
    );
  }
  BoardState(const std::vector<char>& castlingRights, Color playerTurn, std::optional<int> enpassantSq) : 
    state_{encode(castlingRights, playerTurn, enpassantSq)} {};
  BoardState(const std::string& fen) {
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

  [[nodiscard]] int castlingBits() const { return (state_ >> BIT_OFFSETS::CASTLING_PRIVS) & 0xf; }
  [[nodiscard]] bool isCastlingRightAvailable(char right) const { return castlingBits() & (1 << PRIV_BIT_OFFSET(right)); }
  [[nodiscard]] bool blackToMove() const { return (state_ >> BIT_OFFSETS::TURN) & 0x1; }
  [[nodiscard]] Color getTurn() const { return Color(!blackToMove()); }
  [[nodiscard]] Color getOpposition() const { return Color(blackToMove()); }
  [[nodiscard]] std::optional<int> getEnpassantSquare() const { return existsEnpassantSq() ? std::optional<int>(enpassantSq()) : std::nullopt; }

  [[nodiscard]] std::string toString() const {
    std::stringstream ss;
    for (char right : { 'k', 'q', 'K', 'Q' })
      if (isCastlingRightAvailable(right))
        ss << right;
    if (ss.str().empty())
      ss << "-";
    ss << (blackToMove() ? " b " : " w ");
    ss << (existsEnpassantSq() ? indexToAlgebraicNotation(*getEnpassantSquare()) : "-");
    return ss.str();  
  }

  template<char CastlingRight, char... RemainingRights>
  void stripCastlingPrivileges() {
    state_ &= ~(1 << (CastlingTraits<CastlingRight>::bit_offset + BIT_OFFSETS::CASTLING_PRIVS));
    if constexpr (sizeof...(RemainingRights) > 0) {
      stripCastlingPrivileges<RemainingRights...>();
    }
  }
  void setEnpassantSquare(std::optional<int> square) {
    // clear previous enpassant square
    state_ &= ~(1 << BIT_OFFSETS::EXISTS_ENPASSANT_SQ);
    state_ &= ~(0x3f << BIT_OFFSETS::ENPASSANT_SQ);
    // set new enpassant square
    state_ |= static_cast<u32>(square.has_value()) << BIT_OFFSETS::EXISTS_ENPASSANT_SQ;
    state_ |= static_cast<u32>(square.value_or(0)) << BIT_OFFSETS::ENPASSANT_SQ;
  }
  void updateTurn() { state_ ^= 1 << BIT_OFFSETS::TURN; }

  // optimized to avoid use of unordered_map, wrote custom hash function that perfectly maps chars to 
  // respective castling destination
  std::vector<int> availableCastlingDestinations(Color color) const {
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

  [[nodiscard]] u32 extract() const { return state_; }
  void revert(u32 state) { state_ = state; }

  bool operator==(const BoardState& other) const {
    return castlingBits() == other.castlingBits() && getTurn() == other.getTurn()
        && getEnpassantSquare().value_or(0) == other.getEnpassantSquare().value_or(0);
  }
  bool operator!=(const BoardState& other) const { return ! (operator==(other)); }
};

