#pragma once

#include <array>

#include "board/bitboards.h"
#include "board/pieces.h"
#include "util/platform.h"

class Evaluator
{
    template <Color color>
    static int evaluateSide(const Bitboards& bitboards) noexcept;

    // used in evaluate_v1 and evaluate_v2 for benchmarking/debugging
    static int evaluateSide_v1(const Bitboards& bitboards, Color color) noexcept;
    static int evaluateSide_v2(const Bitboards& bitboards, Color color) noexcept;

  public:
    static int evaluate(const Bitboards& bitboards) noexcept;

    // used for benchmarking/debugging
    static int evaluate_v1(const Bitboards& bitboards) noexcept;
    static int evaluate_v2(const Bitboards& bitboards) noexcept;
};
