#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <unordered_set>

#include "board/constants.h"
#include "board/pieces.h"
#include "board/position.h"
#include "board/squares.h"
#include "move/move.h"
#include "move/uci.h"
#include "test_utils.h"

class MoveEncodingTest : public ChessTestFixture
{
};

TEST_F(MoveEncodingTest, PacksEveryFieldWithoutOverlap)
{
    for (const MoveType type : {MoveType::Normal, MoveType::Enpassant, MoveType::Promotion,
                                MoveType::Castle, MoveType::DoublePawnPush})
    {
        for (const Color side : {Color::WHITE, Color::BLACK})
        {
            for (const PieceType captured :
                 {PieceType::NONE, PieceType::PAWN, PieceType::ROOK, PieceType::QUEEN})
            {
                const Move move(type, Square::B7, Square::C8, side, PieceType::PAWN, captured);

                EXPECT_EQ(move.start(), Square::B7);
                EXPECT_EQ(move.end(), Square::C8);
                EXPECT_EQ(move.type(), type);
                EXPECT_EQ(move.side(), side);
                EXPECT_EQ(move.oppSide(), opposite(side));
                EXPECT_EQ(move.moved(), PieceType::PAWN);
                EXPECT_EQ(move.captured(), captured);
                EXPECT_EQ(move.isCapture(), captured != PieceType::NONE);
            }
        }
    }
}

TEST_F(MoveEncodingTest, PacksEverySquarePair)
{
    for (int from = 0; from < SQUARES; ++from)
    {
        for (int to = 0; to < SQUARES; ++to)
        {
            const Move move(MoveType::Normal, from, to, Color::WHITE, PieceType::KNIGHT);
            EXPECT_EQ(move.start(), from);
            EXPECT_EQ(move.end(), to);
        }
    }
}

TEST_F(MoveEncodingTest, PromotionPieceSurvivesPacking)
{
    for (const PieceType promotionPiece : PROMOTION_PIECES)
    {
        const Move move(MoveType::Promotion, Square::A7, Square::A8, Color::WHITE, PieceType::PAWN,
                        std::nullopt, promotionPiece);

        EXPECT_EQ(move.promotionPiece(), promotionPiece);
        EXPECT_EQ(move.uci(), std::string("a7a8") + promotionPieceToChar(promotionPiece));
    }
}

// Non-promotions canonicalize the promotion bits to zero, so equality and the
// ordering key stay exact for move types that never promote.
TEST_F(MoveEncodingTest, NonPromotionsIgnoreThePromotionArgument)
{
    const Move queenArg(MoveType::Normal, Square::E2, Square::E4, Color::WHITE, PieceType::PAWN,
                        std::nullopt, PieceType::QUEEN);
    const Move knightArg(MoveType::Normal, Square::E2, Square::E4, Color::WHITE, PieceType::PAWN,
                         std::nullopt, PieceType::KNIGHT);

    EXPECT_EQ(queenArg, knightArg);
    EXPECT_EQ(queenArg.orderingKey(), knightArg.orderingKey());
    EXPECT_EQ(queenArg.uci(), "e2e4");
}

TEST_F(MoveEncodingTest, OrderingKeyDistinguishesPromotionsAndFitsItsWidth)
{
    std::unordered_set<u16> keys;
    for (const PieceType promotionPiece : PROMOTION_PIECES)
    {
        const Move move(MoveType::Promotion, Square::A7, Square::A8, Color::WHITE, PieceType::PAWN,
                        std::nullopt, promotionPiece);

        EXPECT_LT(move.orderingKey(), 1u << Move::ORDERING_KEY_WIDTH);
        keys.insert(move.orderingKey());
    }

    EXPECT_EQ(keys.size(), PROMOTION_PIECES.size());
}

// MoveOrdering treats a zeroed ordering key as "no transposition table move",
// which is only safe because no real move starts and ends on the same square.
TEST_F(MoveEncodingTest, NoLegalMoveHasAZeroedOrderingKey)
{
    for (const std::string& fen :
         {std::string(STARTING_FEN),
          std::string("1n1r1b1r/P1P1P1P1/2BNq1k1/7R/3Q4/1P1N2K1/P1PBP3/5R2 w - - 15 45"),
          std::string("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1")})
    {
        loadFen(fen);
        for (const Move move : legalMoves())
        {
            EXPECT_NE(move.orderingKey(), 0u) << move.uci() << " in " << fen;
            EXPECT_NE(move.start(), move.end()) << move.uci() << " in " << fen;
        }
    }
}

TEST_F(MoveEncodingTest, UciRoundTripsUnderpromotions)
{
    loadFen("1n1r1b1r/P1P1P1P1/2BNq1k1/7R/3Q4/1P1N2K1/P1PBP3/5R2 w - - 15 45");

    // a7a8 is a quiet promotion, a7b8 captures the knight.
    for (const std::string& uci : {"a7a8q", "a7a8r", "a7a8b", "a7a8n", "a7b8q", "a7b8r", "a7b8b",
                                   "a7b8n"})
    {
        const Move parsed = uciToMove(uci, pos);

        EXPECT_EQ(parsed.uci(), uci);
        EXPECT_EQ(parsed.type(), MoveType::Promotion);
        EXPECT_EQ(parsed.promotionPiece(), charToPromotionPiece(uci[4]));
        EXPECT_TRUE(legalMoves().contains(parsed)) << uci;
    }
}

TEST_F(MoveEncodingTest, UnderpromotionAppliesAndUndoes)
{
    loadFen("1n1r1b1r/P1P1P1P1/2BNq1k1/7R/3Q4/1P1N2K1/P1PBP3/5R2 w - - 15 45");
    const Position original = pos;

    for (const std::string& uci : {"a7a8n", "a7b8r", "c7c8b", "c7d8n"})
    {
        const Move move = uciToMove(uci, pos);
        const auto snapshot = pos.getStateSnapshot();

        pos.applyMove(move);
        EXPECT_EQ(pos.getPieceOccupyingSquare(move.end()), move.promotionPiece()) << uci;
        EXPECT_NE(pos, original) << uci;

        pos.undoMove(move, snapshot);
        EXPECT_EQ(pos, original) << uci;
    }
}
