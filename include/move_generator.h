#pragma once

#include <iostream>

#include "move_factory.h"
#include "pieces.h"
#include "position.h"

class MoveGenerator
{
  public:
    template <Color color>
    static void pushLegalMoves(const Position& pos, MoveList& moves) noexcept
    {
        // push normal moves
        pushLegalMoves<MoveType::Normal, PieceType::PAWN, color>(pos, moves);
        pushLegalMoves<MoveType::Normal, PieceType::KNIGHT, color>(pos, moves);
        pushLegalMoves<MoveType::Normal, PieceType::ROOK, color>(pos, moves);
        pushLegalMoves<MoveType::Normal, PieceType::BISHOP, color>(pos, moves);
        pushLegalMoves<MoveType::Normal, PieceType::QUEEN, color>(pos, moves);
        pushLegalMoves<MoveType::Normal, PieceType::KING, color>(pos, moves);

        // push special moves
        pushLegalMoves<MoveType::Promotion, PieceType::PAWN, color>(pos, moves);
        pushLegalMoves<MoveType::Enpassant, PieceType::PAWN, color>(pos, moves);
        pushLegalMoves<MoveType::DoublePawnPush, PieceType::PAWN, color>(pos, moves);
        pushLegalMoves<MoveType::Castle, PieceType::KING, color>(pos, moves);
    }
    template <MoveType mType, PieceType pType, Color color>
    static void pushLegalMoves(const Position& pos, MoveList& moves) noexcept
    {
        if constexpr (mType == MoveType::Normal && pType != PieceType::PAWN)
        {
            BitUtils::bitsForEach<>(
                pos.getPieceBitboard(pType, color),
                [&](int start) noexcept
                {
                    BitUtils::bitsForEach<>(
                        pos.clearAllySquares(attackedSquares<pType>(pos, start), color),
                        [&](int dest) noexcept
                        {
                            pushIfLegal<mType, color>(
                                pos, MoveFactory::createMove<mType, pType, color>(pos, start, dest),
                                moves);
                        });
                });
        }
        else if constexpr (mType == MoveType::Normal && pType == PieceType::PAWN)
            pushLegalNormalPawnMoves<color>(pos, moves);
        else if constexpr (mType == MoveType::Promotion && pType == PieceType::PAWN)
            pushLegalPromotionMoves<color>(pos, moves);
        else if constexpr (mType == MoveType::Enpassant && pType == PieceType::PAWN)
            pushLegalEnpassantMoves<color>(pos, moves);
        else if constexpr (mType == MoveType::DoublePawnPush && pType == PieceType::PAWN)
            pushLegalDoublePawnPushMoves<color>(pos, moves);
        else if constexpr (mType == MoveType::Castle && pType == PieceType::KING)
            pushLegalCastleMoves<color>(pos, moves);
        else
            static_assert(false, "Invalid move type and piece type combination");
    }

    /////////////////////////
    // Move Validation     //
    /////////////////////////
    template <MoveType mType, Color color>
    static bool isMoveLegal(const Position& pos, const Move<mType> move) noexcept
    {
        if constexpr (mType == MoveType::Castle)
        {
            // only check valid castle when making move so assume color is the side to move
            int king = pos.kingSquare(color);
            u64 emptySquaresMask =
                king < move.end() ? QUEENSIDE_CASTLE_MASK(color) : KINGSIDE_CASTLE_MASK(color);
            // piece in between the rook and king so no castle
            if (pos.filterOccupiedSquares(emptySquaresMask))
                return false;
            // create bitboard mask for all squares that need to be safe
            u64 safetyMask =
                (1ULL << king) | (1ULL << ((king + move.end()) / 2)) | (1ULL << move.end());
            return isBitboardSafeFromSide<opposite<color>()>(pos, safetyMask);
        }
        else
        {
            auto tmp = pos;
            tmp.applyMove(move);
            return !isKingInCheck<color>(tmp);
        }
    }

    template <Color color>
    static bool isKingInCheck(const Position& pos) noexcept
    {
        return !isBitboardSafeFromSide<opposite<color>()>(
            pos, pos.getPieceBitboard(PieceType::KING, color));
    }

  private:
    //////////////////////////
    // Move Operations      //
    //////////////////////////
    template <MoveType mType, Color color>
    static bool pushIfLegal(const Position& pos, Move<mType> candidateMove,
                            MoveList& moves) noexcept
    {
        if (!isMoveLegal<mType, color>(pos, candidateMove))
            return false;
        return moves.push(candidateMove), true;
    }

    /////////////////////////
    // Safety Checks       //
    /////////////////////////
    template <Color color>
    static bool isBitboardSafeFromSide(const Position& pos, u64 bb) noexcept
    {
        return !isBitboardAttackedBySide<color, PieceType::PAWN, PieceType::KNIGHT,
                                         PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN,
                                         PieceType::KING>(pos, bb);
    }

    template <Color color>
    static bool isSquareSafeFromSide(const Position& pos, int square) noexcept
    {
        return isBitboardSafeFromSide<color>(pos, 1ULL << square);
    }

    template <Color color, PieceType pType, PieceType... rest>
    static bool isBitboardAttackedBySide(const Position& pos, u64 bb) noexcept
    {
        if constexpr (sizeof...(rest) > 0)
            return (bb & controlledSquares<color, pType>(pos)) ||
                   isBitboardAttackedBySide<color, rest...>(pos, bb);
        return (bb & controlledSquares<color, pType>(pos));
    }

    /////////////////////////
    // Attack Generation   //
    /////////////////////////
    template <Color color, PieceType pType, PieceType... rest>
    static u64 controlledSquares(const Position& pos) noexcept
    {
        u64 ctrlSquares = rawControlledSquares<color, pType>(pos);
        if constexpr (sizeof...(rest) > 0)
            ctrlSquares |= controlledSquares<color, rest...>(pos);
        return ctrlSquares;
    }

    // Raw controlled squares (all squares piece can attack, including allies)
    template <Color color, PieceType pType>
    static u64 rawControlledSquares(const Position& pos) noexcept
    {
        if constexpr (pType == PieceType::PAWN)
        {
            return squaresAttackedByPawns<color>(pos);
        }
        else
        {
            return BitUtils::accumulateBits<u64>(
                pos.getPieceBitboard(pType, color), [&](u64 targets, int lsb) noexcept
                { return targets | attackedSquares<pType>(pos, lsb); });
        }
    }

    template <Color color>
    static inline u64 singlePawnPushTargets(const Position& pos) noexcept
    {
        return pos.clearOccupiedSquares(
            stepBitboard<forward<color>>(pos.getPieceBitboard(PieceType::PAWN, color)));
    }

    template <Color color>
    static inline u64 squaresAttackedByPawns(const Position& pos) noexcept
    {
        return stepBitboard<leftPawnAttack<color>, rightPawnAttack<color>>(
            pos.getPieceBitboard(PieceType::PAWN, color),
            [](u64 bb) noexcept { return BitUtils::clearFile<'a'>(bb); },
            [](u64 bb) noexcept { return BitUtils::clearFile<'h'>(bb); });
    }

    /////////////////////////
    // Sliding Attacks     //
    /////////////////////////
    [[nodiscard]] static u64 attackedDiagonalSquares(const Position& pos, int square) noexcept
    {
        return attackedSquaresAlongLaneFromSquare<Direction::NW>(pos, square) |
               attackedSquaresAlongLaneFromSquare<Direction::NE>(pos, square) |
               attackedSquaresAlongLaneFromSquare<Direction::SE>(pos, square) |
               attackedSquaresAlongLaneFromSquare<Direction::SW>(pos, square);
    }

    [[nodiscard]] static u64 attackedOrthogonalSquares(const Position& pos, int square) noexcept
    {
        return attackedSquaresAlongLaneFromSquare<Direction::N>(pos, square) |
               attackedSquaresAlongLaneFromSquare<Direction::E>(pos, square) |
               attackedSquaresAlongLaneFromSquare<Direction::S>(pos, square) |
               attackedSquaresAlongLaneFromSquare<Direction::W>(pos, square);
    }

    template <PieceType pType>
    [[nodiscard]] static u64 attackedSquares(const Position& pos, int square) noexcept
    {
        static_assert(pType != PieceType::PAWN,
                      "Pawns are not supported in this function; use this for other piece types");
        if constexpr (pType == PieceType::KNIGHT)
            return Attacks::getKnightAttackBitmap(square);
        else if constexpr (pType == PieceType::KING)
            return Attacks::getKingAttackBitmap(square);
        else if constexpr (pType == PieceType::BISHOP)
            return attackedDiagonalSquares(pos, square);
        else if constexpr (pType == PieceType::ROOK)
            return attackedOrthogonalSquares(pos, square);
        else // queen
            return attackedDiagonalSquares(pos, square) | attackedOrthogonalSquares(pos, square);
    }

    // sliding attack helpers
    template <Direction upwardLane>
        requires(Directions::isUpwards(upwardLane))
    static u64 attackedSquaresAlongLaneFromSquare(const Position& pos, int square) noexcept
    {
        u64 attackRay = Attacks::getSlidingAttackBitmap(square, upwardLane);
        u64 piecesOnLane = pos.filterOccupiedSquares(attackRay);
        return attackRay &
               ~Attacks::getSlidingAttackBitmap(BitUtils::ctz(piecesOnLane), upwardLane);
    }

    template <Direction downwardLane>
        requires(!Directions::isUpwards(downwardLane))
    static u64 attackedSquaresAlongLaneFromSquare(const Position& pos, int square) noexcept
    {
        u64 attackRay = Attacks::getSlidingAttackBitmap(square, downwardLane);
        u64 piecesOnLane = pos.filterOccupiedSquares(attackRay);
        return attackRay &
               ~Attacks::getSlidingAttackBitmap(
                   63 ^ BitUtils::clz(piecesOnLane | (piecesOnLane == 0)), downwardLane);
    }

    /////////////////////////
    // Bitboard Utilities  //
    /////////////////////////
    // useful default unary identity function (bitboard => bitboard) for templates
    static constexpr inline auto identity = [](u64 bb) noexcept { return bb; };

    // bitboard shift helpers
    template <Direction direction, Direction... directions,
              typename UnaryOp = decltype(MoveGenerator::identity), typename... UnaryOps>
        requires(std::is_invocable_v<UnaryOp, u64> &&
                 requires(u64 bb, UnaryOp f) {
                     { f(bb) } -> std::same_as<u64>;
                 })
    [[nodiscard]] static u64 stepBitboard(u64 bb, UnaryOp func = MoveGenerator::identity,
                                          UnaryOps... funcs) noexcept
    {
        u64 steppedBB = BitUtils::stepBitsForward(func(bb), direction);
        if constexpr (sizeof...(directions) > 0)
            return steppedBB | stepBitboard<directions...>(bb, funcs...);
        return steppedBB;
    }

    /////////////////////////
    // Pawn Move Helpers   //
    /////////////////////////
    template <Color color, MoveType mType = MoveType::Normal>
    static void pushPawnAttackMoves(const Position& pos, int dest, MoveList& moves) noexcept
    {
        int leftAtkSquare = dest - Directions::sfamt(leftPawnAttack<color>);
        int rightAtkSquare = dest - Directions::sfamt(rightPawnAttack<color>);
        if (!isSquareOnRightEdge(dest) &&
            pos.isPieceOccupyingSquare(PieceType::PAWN, color, leftAtkSquare))
            pushIfLegal<mType, color>(
                pos,
                MoveFactory::createMove<mType, PieceType::PAWN, color>(pos, leftAtkSquare, dest),
                moves);
        if (!isSquareOnLeftEdge(dest) &&
            pos.isPieceOccupyingSquare(PieceType::PAWN, color, rightAtkSquare))
            pushIfLegal<mType, color>(
                pos,
                MoveFactory::createMove<mType, PieceType::PAWN, color>(pos, rightAtkSquare, dest),
                moves);
    }

    template <Color color>
    static void pushLegalNormalPawnMoves(const Position& pos, MoveList& moves) noexcept
    {
        BitUtils::bitsForEach<>(
            BitUtils::clearRank<promotionRank<color>>(singlePawnPushTargets<color>(pos)),
            [&](int dest) noexcept
            {
                pushIfLegal<MoveType::Normal, color>(
                    pos,
                    MoveFactory::createMove<MoveType::Normal, PieceType::PAWN, color>(
                        pos, dest - Directions::sfamt(forward<color>), dest),
                    moves);
            });
        BitUtils::bitsForEach<>(
            pos.filterEnemySquares(
                BitUtils::clearRank<promotionRank<color>>(squaresAttackedByPawns<color>(pos)),
                color),
            [&](int dest) noexcept
            { pushPawnAttackMoves<color, MoveType::Normal>(pos, dest, moves); });
    }

    template <Color color>
    static void pushLegalPromotionMoves(const Position& pos, MoveList& moves) noexcept
    {
        BitUtils::bitsForEach<>(
            BitUtils::filterRank<promotionRank<color>>(singlePawnPushTargets<color>(pos)),
            [&](int dest) noexcept
            {
                pushIfLegal<MoveType::Promotion, color>(
                    pos,
                    MoveFactory::createMove<MoveType::Promotion, PieceType::PAWN, color>(
                        pos, dest - Directions::sfamt(forward<color>), dest),
                    moves);
            });
        BitUtils::bitsForEach<>(
            pos.filterEnemySquares(
                BitUtils::filterRank<promotionRank<color>>(squaresAttackedByPawns<color>(pos)),
                color),
            [&](int dest) noexcept
            { pushPawnAttackMoves<color, MoveType::Promotion>(pos, dest, moves); });
    }

    template <Color color>
    static void pushLegalEnpassantMoves(const Position& pos, MoveList& moves) noexcept
    {
        if (!pos.maybeEnpassantSquare())
            return;
        pushPawnAttackMoves<color, MoveType::Enpassant>(pos, pos.maybeEnpassantSquare().value(),
                                                        moves);
    }

    template <Color color>
    static void pushLegalDoublePawnPushMoves(const Position& pos, MoveList& moves) noexcept
    {
        BitUtils::bitsForEach<>(
            pos.clearOccupiedSquares(stepBitboard<forward<color>>(
                singlePawnPushTargets<color>(pos), [](u64 bb) noexcept
                { return BitUtils::filterRank<pawnRank<color> + rankDelta<color>>(bb); })),
            [&](int dest) noexcept
            {
                pushIfLegal<MoveType::DoublePawnPush, color>(
                    pos,
                    MoveFactory::createMove<MoveType::DoublePawnPush, PieceType::PAWN, color>(
                        pos, dest - 2 * Directions::sfamt(forward<color>), dest),
                    moves);
            });
    }

    template <Color color>
    static void pushLegalCastleMoves(const Position& pos, MoveList& moves) noexcept
    {
        auto castlingDests = pos.availableCastlingDests<color>();
        u64 castlingTargets =
            std::accumulate(castlingDests.begin(), castlingDests.end(), 0ULL,
                            [](u64 dests, int sq) noexcept { return dests | (1ULL << sq); });
        BitUtils::bitsForEach<>(
            castlingTargets,
            [&](int dest) noexcept
            {
                pushIfLegal<MoveType::Castle, color>(
                    pos,
                    MoveFactory::createMove<MoveType::Castle, PieceType::KING, color>(
                        pos, pos.kingSquare(color), dest),
                    moves);
            });
    }
};
