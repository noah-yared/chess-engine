#pragma once

#include <array>

#include "board/bitboards.h"
#include "board/pieces.h"
#include "util/platform.h"

class Evaluator
{
    template <Color color>
    static int evaluateSide(const Bitboards& bitboards) noexcept;

  public:
    static int evaluate(const Bitboards& bitboards) noexcept;
};
