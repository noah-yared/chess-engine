#pragma once

#include <array>

#include "board_state.h"
#include "constants.h"
#include "platform.h"

template <typename StateType, size_t MaxDepth = 16>
class StateStack
{
    std::array<StateType, MaxDepth> stack_;
    size_t ply_;

  public:
    StateStack() noexcept : ply_{0} {};

    // observers
    [[nodiscard]] inline bool isEmpty() const noexcept { return ply_ == 0; }
    [[nodiscard]] inline StateType top() const noexcept { return stack_.at(ply_ - 1); }
    [[nodiscard]] inline size_t depth() const noexcept { return ply_; }

    // mutators
    inline void clear() noexcept { ply_ = 0; }
    inline void push(StateType oldState) noexcept { stack_.at(ply_++) = oldState; }
    [[nodiscard]] inline StateType pop() noexcept { return stack_.at(--ply_); }
};
