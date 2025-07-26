#pragma once

#include <array>
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
private:
  // TODO: use a vector instead of an array
  std::array<MoveVariant, MAX_POSSIBLE_LEGAL_MOVES> moveBuffer_{};
  size_t sz_ = 0;

public:
  // observers
  [[nodiscard]] size_t size() const { return sz_; }
  [[nodiscard]] bool isEmpty() const { return sz_ == 0; }

  // mutators
  void clear() { sz_ = 0; }
  MoveVariant& pop() { return moveBuffer_[--sz_]; }

  MoveList& push(const MoveVariant& move) {
    moveBuffer_[sz_++] = move;
    return *this;
  }

  // iterator interface
  [[nodiscard]] MoveVariant* begin() { return moveBuffer_.data(); }
  [[nodiscard]] MoveVariant* end() { return moveBuffer_.data() + sz_; }
  [[nodiscard]] const MoveVariant* begin() const { return moveBuffer_.data(); }
  [[nodiscard]] const MoveVariant* end() const { return moveBuffer_.data() + sz_; }

};
