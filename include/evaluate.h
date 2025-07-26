#pragma once

#include <array>

#include "bitboards.h"
#include "platform.h"
#include "pieces.h"

class Evaluator { 
  static int evaluateSide(const Bitboards& bitboards, Color color); 
public:
  static int evaluate(const Bitboards& bitboards); 
};
