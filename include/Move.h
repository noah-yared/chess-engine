#ifndef MoveEncoding_H
#define MoveEncoding_H

#include "flags.h"
#include "pieces.h"
#include "sides.h"

#include <iostream>
#include <cassert>

namespace MoveEncoding {
enum BIT_OFFSETS {
    START = 0,
    END = 6,
    MOVED_PIECE = 12,
    CAPTURED_PIECE = 16,
    FLAGS = 20, /* at the very front as number of flags may change */
};
}

class Move {
    uint32_t bitfield;

    int flag() const { return bitfield >> MoveEncoding::BIT_OFFSETS::FLAGS; }

    public:
        // null move
        Move(): bitfield(0) {};
        Move(int start, int end, Pieces::piece moved, Flags::flag flags = Flags::NONE, Pieces::piece captured = Pieces::piece::NONE)
            : bitfield( start                      << MoveEncoding::BIT_OFFSETS::START 
                      | end                        << MoveEncoding::BIT_OFFSETS::END 
                      | static_cast<int>(moved)    << MoveEncoding::BIT_OFFSETS::MOVED_PIECE
                      | static_cast<int>(captured) << MoveEncoding::BIT_OFFSETS::CAPTURED_PIECE
                      | flags                      << MoveEncoding::BIT_OFFSETS::FLAGS) {};
        
        int i() const { return (bitfield >> MoveEncoding::BIT_OFFSETS::START) & 0x3f; }
        int f() const { return (bitfield >> MoveEncoding::BIT_OFFSETS::END) & 0x3f; }

        Pieces::piece p() const { return static_cast<Pieces::piece>((bitfield >> MoveEncoding::BIT_OFFSETS::MOVED_PIECE) & 0xf); }
        Pieces::piece capturedPiece() const { return static_cast<Pieces::piece>((bitfield >> MoveEncoding::BIT_OFFSETS::CAPTURED_PIECE) & 0xf); }

        void setFlag(Flags::flag f) { bitfield |= (static_cast<int>(f) << MoveEncoding::BIT_OFFSETS::FLAGS); }
        void setCaptured(Pieces::piece p) { bitfield |= (static_cast<int>(p) << MoveEncoding::BIT_OFFSETS::CAPTURED_PIECE); }

        bool isCapture() const { return flag() & static_cast<int>(Flags::CAPTURE); }
        bool isEnpassant() const { return flag() & static_cast<int>(Flags::ENPASSANT); }
        bool isPromotion() const { return flag() & static_cast<int>(Flags::PROMOTION); } 
        bool isCastle() const { return flag() & static_cast<int>(Flags::CASTLE); }
        bool isDoublePawnPush() const { return flag() & static_cast<int>(Flags::DOUBLESTEP); }
        bool isCheck() const { return flag() & static_cast<int>(Flags::CHECK); }
};

#endif