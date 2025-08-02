#pragma once

#include <algorithm>
#include <array>
#include <unordered_set>
#include <variant>

#include "constants.h"
#include "move.h"
#include "platform.h"

// make MoveVariant public so that it can be used in search
using MoveVariant = std::variant<
  Move<MoveType::Normal>,
  Move<MoveType::Enpassant>,
  Move<MoveType::Promotion>,
  Move<MoveType::Castle>,
  Move<MoveType::DoublePawnPush>
>;

struct MoveList {
  friend std::ostream& operator<<(std::ostream& os, const MoveList& ml);

private:
  std::array<MoveVariant, MAX_POSSIBLE_LEGAL_MOVES> moveBuffer_{};
  size_t sz_ = 0;

public:
  // Constructors 
  MoveList() = default;
  MoveList(const MoveList&) = default;

  template<typename... Args>
  MoveList(Args... moves) : moveBuffer_{moves...}, sz_{sizeof...(moves)} {};

  // Assignment operators
  MoveList& operator=(const MoveList&) = default;
  MoveList& operator=(MoveList&&) = default;

  // Observers
  [[nodiscard]] size_t size() const { return sz_; }
  [[nodiscard]] bool isEmpty() const { return sz_ == 0; }
  [[nodiscard]] const MoveVariant& operator[](size_t i) noexcept { return moveBuffer_[i]; }
  
  // Complex search methods moved to source file
  [[nodiscard]] bool contains(const MoveVariant& move) const;
  [[nodiscard]] bool contains(const std::string& uci) const;

  // mutators
  void clear() { sz_ = 0; }
  MoveVariant& pop() { return moveBuffer_[--sz_]; }
  void push(const MoveVariant& move) { moveBuffer_[sz_++] = move; }

  // iterator interface
  [[nodiscard]] MoveVariant* begin() { return moveBuffer_.data(); }
  [[nodiscard]] MoveVariant* end() { return moveBuffer_.data() + sz_; }
  [[nodiscard]] const MoveVariant* begin() const { return moveBuffer_.data(); }
  [[nodiscard]] const MoveVariant* end() const { return moveBuffer_.data() + sz_; }

  // Comparison operators
  bool operator==(const MoveList& other) const;
  bool operator!=(const MoveList& other) const;
};
