#include <algorithm>
#include <unordered_set>
#include <vector>

#include "move/move_list.h"

// Search methods
bool MoveList::contains(const Move move) const noexcept
{
    return std::find(begin(), end(), move) != end();
}

bool MoveList::contains(const std::string& uci) const noexcept
{
    return std::any_of(begin(), end(),
                       [&uci](const Move move) noexcept { return move.uci() == uci; });
}

std::optional<Move> MoveList::findMove(const std::pair<int, int> step) const noexcept
{
    auto it = std::find_if(begin(), end(),
                           [step](const Move move) noexcept
                           { return move.start() == step.first && move.end() == step.second; });
    if (it == end())
    {
        return std::nullopt;
    }
    return *it;
}

// Comparison operators
bool MoveList::operator==(const MoveList& other) const noexcept
{
    if (sz_ != other.sz_)
        return false;
    std::unordered_set<Move> moveSet(begin(), end()), otherSet(other.begin(), other.end());
    return moveSet == otherSet;
}

bool MoveList::operator!=(const MoveList& other) const noexcept { return !operator==(other); }

// I/O operator
std::ostream& operator<<(std::ostream& os, const MoveList& ml) noexcept
{
    os << "MoveList(";
    std::vector<std::string> ucis(ml.size());
    std::transform(ml.begin(), ml.end(), ucis.begin(),
                   [](const Move move) noexcept { return move.uci(); });
    std::sort(ucis.begin(), ucis.end()); // sort the moves to make it easier to compare movelists
    int movesPrinted = 0;
    for (const auto& uci : ucis)
        os << (movesPrinted ? "," : "") << ((movesPrinted++ % 10 == 0) ? "\n   " : " ") << uci;
    os << "\n)\n";
    return os;
}
