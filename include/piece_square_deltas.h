#pragma once

#include <cassert>
#include <optional>

#include "bitboards.h"
#include "board_state.h"
#include "delta.h"
#include "move.h"
#include "pieces.h"
#include "static_vector.h"

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

template<MoveType mType>
bool always_false = false;

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
            Delta::Remove(move.movedKey(), move.start()),
            Delta::Place(move.movedKey(), move.end()),
        };
        if (auto maybeCapturedKey = move.capturedKey(); maybeCapturedKey)
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
            Delta::Remove(move.movedKey(), move.start()),
            Delta::Place(move.movedKey(), move.end()),
            Delta::Remove(move.enpassantKey(), move.enpassantSquare()),
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
            Delta::Remove(move.movedKey(), move.start()),
            Delta::Place(move.promotionKey(), move.end()),
        };
        if (auto maybeCapturedKey = move.capturedKey(); maybeCapturedKey)
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
            Delta::Remove(move.movedKey(), move.start()),
            Delta::Place(move.movedKey(), move.end()),
            Delta::Remove(move.castledRookKey(), move.castledRookStart()),
            Delta::Place(move.castledRookKey(), move.castledRookEnd()),
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
            Delta::Remove(move.movedKey(), move.start()),
            Delta::Place(move.movedKey(), move.end()),
        };
    }
};
