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
  friend std::ostream& operator<<(std::ostream& os, const MoveList& ml) noexcept;

private:
  std::array<MoveVariant, MAX_POSSIBLE_LEGAL_MOVES> moveBuffer_{};
  size_t sz_ = 0;

public:
  // Constructors 
  MoveList() noexcept = default;
  MoveList(const MoveList&) noexcept = default;

  template<typename... Args>
  MoveList(Args... moves) noexcept : moveBuffer_{moves...}, sz_{sizeof...(moves)} {};

  // Assignment operators
  MoveList& operator=(const MoveList&) noexcept = default;
  MoveList& operator=(MoveList&&) noexcept = default;

  // Observers
  [[nodiscard]] size_t size() const noexcept { return sz_; }
  [[nodiscard]] bool isEmpty() const noexcept { return sz_ == 0; }
  [[nodiscard]] const MoveVariant& operator[](size_t i) noexcept { return moveBuffer_[i]; }
  
  // Complex search methods moved to source file
  [[nodiscard]] bool contains(const MoveVariant& move) const noexcept;
  [[nodiscard]] bool contains(const std::string& uci) const noexcept;

  // mutators
  void clear() noexcept { sz_ = 0; }
  MoveVariant& pop() noexcept { return moveBuffer_[--sz_]; }
  void push(const MoveVariant& move) noexcept { moveBuffer_[sz_++] = move; }

  // iterator interface
  [[nodiscard]] MoveVariant* begin() noexcept { return moveBuffer_.data(); }
  [[nodiscard]] MoveVariant* end() noexcept { return moveBuffer_.data() + sz_; }
  [[nodiscard]] const MoveVariant* begin() const noexcept { return moveBuffer_.data(); }
  [[nodiscard]] const MoveVariant* end() const noexcept { return moveBuffer_.data() + sz_; }

  // Comparison operators
  bool operator==(const MoveList& other) const noexcept;
  bool operator!=(const MoveList& other) const noexcept;
};
