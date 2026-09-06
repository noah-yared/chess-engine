#include <gtest/gtest.h>

#include <optional>
#include <unordered_set>

#include "board/constants.h"
#include "board/pieces.h"
#include "board/squares.h"
#include "move/move.h"
#include "search/transposition_table.h"

namespace
{

[[nodiscard]] Move normalMove(Square from, Square to)
{
    return Move(MoveType::Normal, from, to, Color::WHITE, PieceType::ROOK);
}

[[nodiscard]] Move promotionMove(Square from, Square to, PieceType promotionPiece)
{
    return Move(MoveType::Promotion, from, to, Color::WHITE, PieceType::PAWN, std::nullopt,
                promotionPiece);
}

} // namespace

// BOUND straddles bytes 1 and 2 of the packed entry, so each value exercises
// the split write/read path rather than a single-byte one.
TEST(TranspositionTableTest, RoundTripsEveryBoundAcrossItsByteBoundary)
{
    for (const Bound bound : {Bound::EXACT, Bound::LOWER, Bound::UPPER})
    {
        const PackedTTEntry entry(0xdeadbeefcafe1234ULL, -42, 9, bound,
                                  normalMove(Square::A1, Square::A8).orderingKey());

        EXPECT_EQ(entry.getBound(), bound);
        EXPECT_EQ(entry.getEval(), -42);
        EXPECT_EQ(entry.getDepth(), 9);
        EXPECT_TRUE(entry.isOccupied());
        EXPECT_TRUE(entry.hasMatchingKey(0xdeadbeefcafe1234ULL));
    }
}

// The depth field was four bits wide, so anything past 15 silently wrapped.
TEST(TranspositionTableTest, RoundTripsDepthPastTheOldFourBitLimit)
{
    for (int depth = 0; depth <= MAX_SEARCH_DEPTH; ++depth)
    {
        const PackedTTEntry entry(0x1ULL, 0, depth, Bound::EXACT,
                                  normalMove(Square::E2, Square::E4).orderingKey());

        EXPECT_EQ(entry.getDepth(), depth) << "depth " << depth;
        EXPECT_TRUE(entry.hasAtLeastDepth(depth)) << "depth " << depth;
    }
}

TEST(TranspositionTableTest, RoundTripsNegativeAndExtremeScores)
{
    for (const int score : {MIN_EVAL, -1, 0, 1, MAX_EVAL})
    {
        const PackedTTEntry entry(0x2ULL, score, 3, Bound::EXACT,
                                  normalMove(Square::E2, Square::E4).orderingKey());

        EXPECT_EQ(entry.getEval(), score) << "score " << score;
    }
}

// The best-move field carries the promotion piece, so the four promotions
// sharing a start/end pair stay distinguishable in the table.
TEST(TranspositionTableTest, BestMoveFieldDistinguishesPromotionPieces)
{
    std::unordered_set<u16> storedKeys;
    for (const PieceType promotionPiece : PROMOTION_PIECES)
    {
        const Move move = promotionMove(Square::A7, Square::A8, promotionPiece);
        const PackedTTEntry entry(0x3ULL, 0, 1, Bound::EXACT, move.orderingKey());

        EXPECT_EQ(entry.getMove(), move.orderingKey())
            << "promoting to " << promotionPieceToChar(promotionPiece);
        storedKeys.insert(entry.getMove());
    }

    EXPECT_EQ(storedKeys.size(), PROMOTION_PIECES.size());
}

// Only the low 56 bits of the hash are stored, so the probe has to compare
// against exactly that many. Masking one bit too wide made every key with bit
// 56 set fail to match the fragment it had just written.
TEST(TranspositionTableTest, MatchesKeysWithBitsSetAboveTheStoredFragment)
{
    for (int bit = 0; bit < 64; ++bit)
    {
        const u64 key = (1ULL << bit) | 0xffULL;
        const PackedTTEntry entry(key, 7, 4, Bound::EXACT,
                                  normalMove(Square::E2, Square::E4).orderingKey());

        EXPECT_TRUE(entry.hasMatchingKey(key)) << "bit " << bit;
    }
}

TEST(TranspositionTableTest, StoresProbesAndClears)
{
    TranspositionTable tt(257);
    const Move move = normalMove(Square::G1, Square::F3);
    constexpr u64 hash = 0xabcdef0123456789ULL;

    EXPECT_FALSE(tt.probe(hash).has_value());

    tt.store(hash, 123, 20, Bound::LOWER, move.orderingKey());

    auto probed = tt.probe(hash);
    ASSERT_TRUE(probed.has_value());
    EXPECT_EQ((*probed)->getEval(), 123);
    EXPECT_EQ((*probed)->getDepth(), 20);
    EXPECT_EQ((*probed)->getBound(), Bound::LOWER);
    EXPECT_EQ((*probed)->getMove(), move.orderingKey());

    tt.clear();
    EXPECT_FALSE(tt.probe(hash).has_value());
}
