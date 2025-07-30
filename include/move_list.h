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
  // TODO: use a vector instead of an array
  std::array<MoveVariant, MAX_POSSIBLE_LEGAL_MOVES> moveBuffer_{};
  size_t sz_ = 0;

public:
  // constructors 
  MoveList() = default;
  MoveList(const MoveList&) = default;

  template<typename... Args>
  MoveList(Args... moves) : moveBuffer_{moves...}, sz_{sizeof...(moves)} {};

  // assignment operators
  MoveList& operator=(const MoveList&) = default;
  MoveList& operator=(MoveList&&) = default;

  // observers
  [[nodiscard]] size_t size() const { return sz_; }
  [[nodiscard]] bool isEmpty() const { return sz_ == 0; }
  [[nodiscard]] bool contains(const MoveVariant& move) const {
    return std::find(begin(), end(), move) != end();
  }
  [[nodiscard]] bool contains(const std::string& uci) const {
    return std::any_of(begin(), end(), [uci](auto&& move) { 
      return std::visit([uci](auto&& arg) { return arg.uci() == uci; }, move);
    });
  }

  // mutators
  void clear() { sz_ = 0; }
  MoveVariant& pop() { return moveBuffer_[--sz_]; }
  void push(const MoveVariant& move) { moveBuffer_[sz_++] = move; }

  // iterator interface
  [[nodiscard]] MoveVariant* begin() { return moveBuffer_.data(); }
  [[nodiscard]] MoveVariant* end() { return moveBuffer_.data() + sz_; }
  [[nodiscard]] const MoveVariant* begin() const { return moveBuffer_.data(); }
  [[nodiscard]] const MoveVariant* end() const { return moveBuffer_.data() + sz_; }

  // comparison operators
  bool operator==(const MoveList& other) const {
    if (sz_ != other.sz_) return false;
    std::unordered_set<MoveVariant> moveSet(begin(), end()), otherSet(other.begin(), other.end());
    return moveSet == otherSet;
  }
  bool operator!=(const MoveList& other) const { return !operator==(other); }
};

inline std::ostream& operator<<(std::ostream& os, const MoveList& ml) {
  os << "MoveList(";
  std::vector<std::string> ucis(ml.size());
  std::transform(ml.begin(), ml.end(), ucis.begin(), [](auto&& move) {
    return std::visit([](auto&& arg){ return arg.uci(); }, move);
  });
  std::sort(ucis.begin(), ucis.end()); // sort the moves to make it easier to compare movelists
  int movesPrinted = 0;
  for (auto uci : ucis)
    os << ((movesPrinted++ % 10 == 0) ? "\n   " : ", ") << uci;
  os << " )\n";
  return os;
}
