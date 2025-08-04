#pragma once

#include <array>

#include "bitboards.h"
#include "platform.h"
#include "pieces.h"

// Eval tags for compile-time selection of static eval implementation
struct EvalV1 {};
struct EvalV2 {};
struct EvalV3 {};

class Evaluator { 
  static int evaluateSide_v1(const Bitboards& bitboards, Color color) noexcept; 
  static int evaluateSide_v2(const Bitboards& bitboards, Color color) noexcept; 
  static int evaluateSide_v3(const Bitboards& bitboards, Color color) noexcept; 

  static int evaluate_v1(const Bitboards& bitboards) noexcept; 
  static int evaluate_v2(const Bitboards& bitboards) noexcept;
  static int evaluate_v3(const Bitboards& bitboards) noexcept;
public:
  template<typename EvalType = EvalV3 /* default to V3, current fastest */>
  static int evaluate(const Bitboards& bitboards) noexcept {
    if constexpr(std::is_same_v<EvalType, EvalV1>) {
      return evaluate_v1(bitboards);
    } else if constexpr(std::is_same_v<EvalType, EvalV2>) {
      return evaluate_v2(bitboards);
    } else {
      return evaluate_v3(bitboards);
    }
  }
};
