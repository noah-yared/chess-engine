#pragma once

#include <cassert>
#include <concepts>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "board_state_snapshot.h"
#include "board_utils.h"
#include "pieces.h"
#include "platform.h"
#include "chess_traits.h"

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

  void setTurn(const std::string& color);
  void setCastlingPrivileges(const std::string& privs);

public:
  BoardState() : state_{encode({'K', 'Q', 'k', 'q'}, Color::WHITE, std::nullopt)} {};
  explicit BoardState(u32 state) : state_{state} {};
  explicit BoardState(const BoardStateSnapshot& snapshot) : state_{snapshot.state} {};
  BoardState(const std::vector<char>& castlingRights, Color playerTurn, std::optional<int> enpassantSq) : 
    state_{encode(castlingRights, playerTurn, enpassantSq)} {};

  BoardState(const std::string& turn, const std::string& castlingRights, const std::string& enpassant);
  BoardState(const std::string& fen);

  // Simple accessors
  [[nodiscard]] int castlingBits() const { return (state_ >> BIT_OFFSETS::CASTLING_PRIVS) & 0xf; }
  [[nodiscard]] bool isCastlingRightAvailable(char right) const { return castlingBits() & (1 << PRIV_BIT_OFFSET(right)); }
  [[nodiscard]] bool blackToMove() const { return (state_ >> BIT_OFFSETS::TURN) & 0x1; }
  [[nodiscard]] Color getTurn() const { return Color(!blackToMove()); }
  [[nodiscard]] Color getOpposition() const { return Color(blackToMove()); }
  [[nodiscard]] std::optional<int> getEnpassantSquare() const { return existsEnpassantSq() ? std::optional<int>(enpassantSq()) : std::nullopt; }

  [[nodiscard]] std::vector<int> availableCastlingDestinations(Color color) const;

  // String parsing methods
  std::string parseCastlingRights() const;
  std::string parseTurn() const;
  std::string parseEnpassantSquare() const;
  std::string toString() const;

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

  [[nodiscard]] u32 extract() const { return state_; }
  void revert(u32 state) { state_ = state; }

  // Complex comparison operators moved to source file
  bool operator==(const BoardState& other) const;
  bool operator!=(const BoardState& other) const;
};

