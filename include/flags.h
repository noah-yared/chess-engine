#ifndef FLAGS_H
#define FLAGS_H

namespace Flags {
enum flag {
    NONE       = 0,        // 000000
    CASTLE     = 1 << 0,   // 000001
    ENPASSANT  = 1 << 1,   // 000010
    PROMOTION  = 1 << 2,   // 000100
    DOUBLESTEP = 1 << 3,   // 001000
    CAPTURE    = 1 << 4,   // 010000
    CHECK      = 1 << 5    // 100000
};
}

#endif