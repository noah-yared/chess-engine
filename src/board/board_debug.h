#pragma once

#include <iostream>

#include "board/constants.h"
#include "util/platform.h"

inline void printBitboard(u64 bb) noexcept
{
    for (int r = 7; r >= 0; --r)
    {
        for (int c = 7; c >= 0; --c)
            std::cout << ((bb & (1ULL << (r * FILES + c))) ? 'X' : '.');
        std::cout << '\n';
    }
}
