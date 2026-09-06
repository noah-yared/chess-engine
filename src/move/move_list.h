#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <unordered_set>

#include "board/constants.h"
#include "move/move.h"
#include "util/platform.h"

struct MoveList
{
    friend std::ostream& operator<<(std::ostream& os, const MoveList& ml) noexcept;

  private:
    std::array<Move, MAX_POSSIBLE_LEGAL_MOVES> moveBuffer_{};
    size_t sz_ = 0;

  public:
    // Constructors
    MoveList() noexcept = default;
    MoveList(const MoveList&) noexcept = default;

    template <typename... Args>
    MoveList(Args... moves) noexcept : moveBuffer_{moves...}, sz_{sizeof...(moves)} {};

    // Assignment operators
    MoveList& operator=(const MoveList&) noexcept = default;
    MoveList& operator=(MoveList&&) noexcept = default;

    // Observers
    [[nodiscard]] size_t size() const noexcept { return sz_; }
    [[nodiscard]] bool isEmpty() const noexcept { return sz_ == 0; }
    [[nodiscard]] Move operator[](size_t i) const noexcept { return moveBuffer_[i]; }

    // search methods
    [[nodiscard]] std::optional<Move> findMove(const std::pair<int, int> step) const noexcept;
    [[nodiscard]] bool contains(const Move move) const noexcept;
    [[nodiscard]] bool contains(const std::string& uci) const noexcept;

    // mutators
    void clear() noexcept { sz_ = 0; }
    Move& pop() noexcept { return moveBuffer_[--sz_]; }
    void push(const Move move) noexcept { moveBuffer_[sz_++] = move; }

    // iterator interface
    [[nodiscard]] Move* begin() noexcept { return moveBuffer_.data(); }
    [[nodiscard]] Move* end() noexcept { return moveBuffer_.data() + sz_; }
    [[nodiscard]] const Move* begin() const noexcept { return moveBuffer_.data(); }
    [[nodiscard]] const Move* end() const noexcept { return moveBuffer_.data() + sz_; }

    // sorting
    template <typename CompFunc>
    void sort(CompFunc&& comp) noexcept
    {
        std::sort(moveBuffer_.begin(), moveBuffer_.begin() + sz_, comp);
    }

    // Comparison operators
    bool operator==(const MoveList& other) const noexcept;
    bool operator!=(const MoveList& other) const noexcept;
};
