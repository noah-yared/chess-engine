#pragma once

#include <cassert>
#include <optional>

#include "board/bitboards.h"
#include "board/board_state.h"
#include "board/pieces.h"
#include "move/move.h"
#include "util/static_vector.h"

/*
 * store following info for piece square updates:
 * @key: piece bitboard index
 * @square: bit index of piece bitboard
 */
struct Delta
{
    int key, square;

    static Delta Place(int key, int square) noexcept { return {key, square}; }
    static Delta Remove(int key, int square) noexcept { return {key, square}; }
};

// aliases for different capacity static vectors
using Deltas2 = StaticVector<Delta, 2>;
using Deltas3 = StaticVector<Delta, 3>;
using Deltas4 = StaticVector<Delta, 4>;

// delta container types (different types use different capacities)
template <MoveType mType>
struct DeltasContainerTrait
{
    using type = Deltas4;
};
template <>
struct DeltasContainerTrait<MoveType::Normal>
{
    using type = Deltas3;
};
template <>
struct DeltasContainerTrait<MoveType::Enpassant>
{
    using type = Deltas3;
};
template <>
struct DeltasContainerTrait<MoveType::Promotion>
{
    using type = Deltas3;
};
template <>
struct DeltasContainerTrait<MoveType::Castle>
{
    using type = Deltas4;
};
template <>
struct DeltasContainerTrait<MoveType::DoublePawnPush>
{
    using type = Deltas2;
};

// alias to reduce verbosity of DeltasContainerTrait<mType>::type
template <MoveType mType>
using DeltasContainerType = DeltasContainerTrait<mType>::type;

template <MoveType mType>
bool always_false = false;

namespace move_delta_detail
{

template <MoveType mType>
[[nodiscard]] inline int movedKey(const Move<mType> move) noexcept
{
    return Bitboards::pieceToKey(move.moved(), move.side());
}

template <MoveType mType>
[[nodiscard]] inline std::optional<int> capturedKey(const Move<mType> move) noexcept
{
    return move.captured() != PieceType::NONE
               ? std::optional<int>(Bitboards::pieceToKey(move.captured(), move.oppSide()))
               : std::nullopt;
}

[[nodiscard]] inline int promotionKey(const Move<MoveType::Promotion> move) noexcept
{
    return Bitboards::pieceToKey(move.promotionPiece(), move.side());
}

[[nodiscard]] inline int castledRookKey(const Move<MoveType::Castle> move) noexcept
{
    return Bitboards::pieceToKey(PieceType::ROOK, move.side());
}

[[nodiscard]] inline int enpassantCaptureKey(const Move<MoveType::Enpassant> move) noexcept
{
    return Bitboards::pieceToKey(PieceType::PAWN, move.oppSide());
}

} // namespace move_delta_detail

template <MoveType mType>
struct PieceSquareDeltas
{
    using MoveDeltasList = DeltasContainerType<mType>;
    static MoveDeltasList generate(const Move<mType> move) noexcept
    {
        static_assert(always_false<mType>, "Invalid move type");
    }
};

template <>
struct PieceSquareDeltas<MoveType::Normal>
{
    using MoveDeltasList = DeltasContainerType<MoveType::Normal>;
    static MoveDeltasList generate(const Move<MoveType::Normal> move) noexcept
    {
        MoveDeltasList deltas = {
            Delta::Remove(move_delta_detail::movedKey(move), move.start()),
            Delta::Place(move_delta_detail::movedKey(move), move.end()),
        };
        if (auto maybeCapturedKey = move_delta_detail::capturedKey(move); maybeCapturedKey)
            deltas.push_back(Delta::Remove(*maybeCapturedKey, move.end()));
        return deltas;
    }
};

template <>
struct PieceSquareDeltas<MoveType::Enpassant>
{
    using MoveDeltasList = DeltasContainerType<MoveType::Enpassant>;
    static MoveDeltasList generate(const Move<MoveType::Enpassant> move)
    {
        return {
            Delta::Remove(move_delta_detail::movedKey(move), move.start()),
            Delta::Place(move_delta_detail::movedKey(move), move.end()),
            Delta::Remove(move_delta_detail::enpassantCaptureKey(move), move.enpassantSquare()),
        };
    }
};

template <>
struct PieceSquareDeltas<MoveType::Promotion>
{
    using MoveDeltasList = DeltasContainerType<MoveType::Promotion>;
    static MoveDeltasList generate(const Move<MoveType::Promotion> move) noexcept
    {
        MoveDeltasList deltas = {
            Delta::Remove(move_delta_detail::movedKey(move), move.start()),
            Delta::Place(move_delta_detail::promotionKey(move), move.end()),
        };
        if (auto maybeCapturedKey = move_delta_detail::capturedKey(move); maybeCapturedKey)
            deltas.push_back(Delta::Remove(*maybeCapturedKey, move.end()));
        return deltas;
    }
};

template <>
struct PieceSquareDeltas<MoveType::Castle>
{
    using MoveDeltasList = DeltasContainerType<MoveType::Castle>;
    static MoveDeltasList generate(const Move<MoveType::Castle> move) noexcept
    {
        return {
            Delta::Remove(move_delta_detail::movedKey(move), move.start()),
            Delta::Place(move_delta_detail::movedKey(move), move.end()),
            Delta::Remove(move_delta_detail::castledRookKey(move), move.castledRookStart()),
            Delta::Place(move_delta_detail::castledRookKey(move), move.castledRookEnd()),
        };
    }
};

template <>
struct PieceSquareDeltas<MoveType::DoublePawnPush>
{
    using MoveDeltasList = DeltasContainerType<MoveType::DoublePawnPush>;
    static MoveDeltasList generate(const Move<MoveType::DoublePawnPush> move) noexcept
    {
        return {
            Delta::Remove(move_delta_detail::movedKey(move), move.start()),
            Delta::Place(move_delta_detail::movedKey(move), move.end()),
        };
    }
};
