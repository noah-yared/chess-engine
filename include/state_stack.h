#pragma once

#include <array>

#include "board_state.h"
#include "constants.h"
#include "platform.h"


template<typename StateType>
class StateStack {
  std::array<StateType, SEARCH_DEPTH+1> stack_;
  size_t ply_;

 public:
  StateStack(): ply_{} {};

  // observers
  [[nodiscard]] inline bool isEmpty() const { return ply_ == 0; }
  [[nodiscard]] inline StateType top() const { return stack_.at(ply_-1); }

  // mutators
  inline void clear() { ply_ = 0; }
  inline void push(StateType oldState) { stack_.at(ply_++) = oldState; }
  [[nodiscard]] inline StateType pop() { return stack_.at(--ply_); }
};
