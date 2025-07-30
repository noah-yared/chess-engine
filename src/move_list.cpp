#include "move_list.h"
#include <algorithm>
#include <unordered_set>

// Search methods
bool MoveList::contains(const MoveVariant& move) const {
  return std::find(begin(), end(), move) != end();
}

bool MoveList::contains(const std::string& uci) const {
  return std::any_of(begin(), end(), [uci](auto&& move) { 
    return std::visit([uci](auto&& arg) { return arg.uci() == uci; }, move);
  });
}

// Comparison operators
bool MoveList::operator==(const MoveList& other) const {
  if (sz_ != other.sz_) return false;
  std::unordered_set<MoveVariant> moveSet(begin(), end()), otherSet(other.begin(), other.end());
  return moveSet == otherSet;
}

bool MoveList::operator!=(const MoveList& other) const { 
  return !operator==(other); 
}

// I/O operator
std::ostream& operator<<(std::ostream& os, const MoveList& ml) {
  os << "MoveList(";
  std::vector<std::string> ucis(ml.size());
  std::transform(ml.begin(), ml.end(), ucis.begin(), [](auto&& move) {
    return std::visit([](auto&& arg){ return arg.uci(); }, move);
  });
  std::sort(ucis.begin(), ucis.end()); // sort the moves to make it easier to compare movelists
  int movesPrinted = 0;
  for (const auto& uci : ucis)
    os << ((movesPrinted++ % 10 == 0) ? "\n   " : ", ") << uci;
  os << " )\n";
  return os;
}
