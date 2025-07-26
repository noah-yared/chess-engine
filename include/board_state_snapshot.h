#pragma once

#include "platform.h"

struct BoardStateSnapshot {
  u32 state; // state of the board at snapshot
  u64 hash; // zobrist hash of the board at snapshot
};
