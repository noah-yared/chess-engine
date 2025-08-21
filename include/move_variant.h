#pragma once

#include <variant>

#include "move.h"

// make MoveVariant public so that it can be used in search
using MoveVariant =
    std::variant<Move<MoveType::Normal>, Move<MoveType::Enpassant>, Move<MoveType::Promotion>,
                 Move<MoveType::Castle>, Move<MoveType::DoublePawnPush>>;
