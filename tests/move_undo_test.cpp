#include <gtest/gtest.h>

#include <string>

#include "move.h"
#include "engine.h"
#include "board_utils.h"
#include "test_utils.h"

class MoveUndoTest : public ChessTestFixture {
protected:
  template<typename... mTypes>
  requires (sizeof...(mTypes) != 0) // should be at least one move
  void testMoveSequence(const mTypes... moves) {
    auto engine = createTestEngine();
    testMoveSequenceImpl(engine, moves...);
  }

private:
  [[nodiscard]] SearchEngine createTestEngine() const {
    return SearchEngine::withEmptyTTForTesting(pos);
  }

  template <typename mType, typename... rest>
  void testMoveSequenceImpl(SearchEngine& engine, const mType move, const rest... moves) {
    auto startingPos = engine.getPosition();
    engine.advance(move);
    if constexpr (sizeof...(moves) > 0)
      testMoveSequenceImpl(engine, moves...);
    engine.backtrack(move);
    EXPECT_EQ(startingPos, engine.getPosition());
  }
};

TEST_F(MoveUndoTest, HandlesPawnPushMove) {
  loadStartingPosition();
  testMoveSequence(normal(Square::A2, Square::A3), normal(Square::A7, Square::A6));

  loadFen("3K4/Pp1p1N1n/bpPppk1P/PN2pp2/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 1");
  testMoveSequence(normal(Square::E5, Square::E4), normal(Square::D4, Square::D5));
}

TEST_F(MoveUndoTest, HandlesPawnCaptureMove) {
  loadFen("3K4/Pp1p1N1n/bpPppk1P/PN2pp2/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 1");
  testMoveSequence(normal(Square::E5, Square::D4), normal(Square::C6, Square::B7),
      normal(Square::D4, Square::C3), normal(Square::D2, Square::C3));
}

TEST_F(MoveUndoTest, HandlesPawnDoublePushMove) {
  loadStartingPosition();
  testMoveSequence(doublePush(Square::A2, Square::A4), doublePush(Square::D7, Square::D5));

  loadFen("1N2br1b/Kp2pR2/n1r1pp1N/P3Bnq1/3P1pp1/4P1Pp/1QP2RBk/8 w - - 1 6");
  testMoveSequence(doublePush(Square::C2, Square::C4), doublePush(Square::B7, Square::B5));
}

TEST_F(MoveUndoTest, HandlesPawnEnpassantMove) {
  loadFen("1N3r1b/K2bpR2/n1r1pp1N/Pp2Bnq1/3P1pp1/2P1P1Pp/2Q2RBk/8 w - b6 0 6");
  testMoveSequence(enpassant(Square::A5, Square::B6));

  loadFen("1N3r1b/K2bpR2/n1r1pp2/P3BNq1/1pPP1pp1/4P1Pp/1Q3RBk/8 b - c3 0 7");
  testMoveSequence(enpassant(Square::B4, Square::C3));

  loadFen("1N3r1b/Kp1b4/n1rB1R1N/P2Ppnq1/5pp1/4P3/1QP2Rpk/8 w - e6 0 11");
  testMoveSequence(enpassant(Square::D5, Square::E6));
}

TEST_F(MoveUndoTest, HandlesPawnPromotionMove) {
  // Recall that engine defaults promotion piece to queen
  loadFen("1N3r1b/Kp1bP3/n1rB1R1N/P4nq1/5pp1/4P3/1QP2Rp1/7k b - - 0 12");
  testMoveSequence(promotion(Square::G2, Square::G1), promotion(Square::E7, Square::F8));

  loadFen("3K4/PP1p1N1n/bp1p1k1P/PN2pp2/1p1p1PBb/2RPB2q/3Pn3/1r1Q2Rr w - - 0 3");
  testMoveSequence(promotion(Square::B7, Square::B8), normal(Square::B4, Square::C3),
      promotion(Square::A7, Square::A8));
}

TEST_F(MoveUndoTest, HandlesKnightCaptureMove) {
  loadFen("1N3Q1b/Kp1b4/n1rB1R1N/P4nq1/5pp1/4P3/1QP2R2/6qk b - - 0 13");
  testMoveSequence(normal(Square::F5, Square::E3), normal(Square::H6, Square::G4),
      normal(Square::E3, Square::G4));

  loadFen("3K4/Pp1p1N2/bpPppk1P/PN2ppn1/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr w - - 1 2");
  testMoveSequence(normal(Square::F7, Square::G5), normal(Square::E2, Square::C3));
}

TEST_F(MoveUndoTest, HandlesKnightNonCaptureMove) {
  loadStartingPosition();
  testMoveSequence(normal(Square::B1, Square::A3), normal(Square::G8, Square::F6));

  loadFen("3K4/Pp1p1N1n/bpPppk1P/PN2pp2/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 1");
  testMoveSequence(normal(Square::H7, Square::G5), normal(Square::B5, Square::C7));
}

TEST_F(MoveUndoTest, HandlesBishopCaptureMove) {
  loadFen("1K6/Bp1p2P1/1qQpBnnp/p1NP4/1PpRppb1/1NP2r1R/PP3P1P/1k1rb3 w - - 1 2");
  testMoveSequence(normal(Square::E6, Square::D7), normal(Square::G4, Square::H3),
      normal(Square::D7, Square::H3), normal(Square::E1, Square::C3), normal(Square::A7, Square::B6));
}

TEST_F(MoveUndoTest, HandlesBishopNonCaptureMove) {
  loadFen("2R4K/R1P1p1p1/1P1B3n/bB3P2/brppP3/nP1p1P1P/k3ppPp/Nq2Q1rN w - - 0 1");
  testMoveSequence(normal(Square::B5, Square::D7), normal(Square::A4, Square::C6),
      normal(Square::D6, Square::F4), normal(Square::C6, Square::A8));
}

TEST_F(MoveUndoTest, HandlesRookCaptureMove) {
  loadFen("b1R4K/R1PBp1p1/1P5n/b4P2/1rppPB2/nP1p1P1P/k3ppPp/Nq2Q1rN w - - 4 3");
  testMoveSequence(normal(Square::C8, Square::A8), normal(Square::G1, Square::E1),
      normal(Square::A7, Square::A5));
}

TEST_F(MoveUndoTest, HandlesRookNonCaptureMove) {
  loadFen("1rbbN3/Kpp2p1Q/1n4nP/BP1BPp2/k3ppPp/3PN2p/2RR1PP1/1q5r w - - 0 2");
  testMoveSequence(normal(Square::C2, Square::C6), normal(Square::H1, Square::C1),
      normal(Square::D2, Square::A2));
}

TEST_F(MoveUndoTest, HandlesQueenCaptureMove) {
  loadFen("1rbbN3/Kpp2p1Q/1nR3nP/BP1BPp2/k3ppPp/3PN2p/R4PP1/1qr5 b - - 3 3");
  testMoveSequence(normal(Square::B1, Square::A2), normal(Square::H7, Square::G6),
      normal(Square::A2, Square::D5), normal(Square::G6, Square::F7), normal(Square::D5, Square::F7));
}

TEST_F(MoveUndoTest, HandlesQueenNonCaptureMove) {
  loadFen("2qb3R/rn3kPB/pR3B1p/P2P2bK/2p1P3/1P1Pp1pP/1NrppPp1/3N2Q1 b - - 0 1");
  testMoveSequence(normal(Square::C8, Square::F5), normal(Square::G1, Square::E1),
      normal(Square::F5, Square::E5));
}

TEST_F(MoveUndoTest, HandlesKingNormalMove) {
  loadFen("1K1N4/R3b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP4Q1/4k3 w - - 0 4");
  testMoveSequence(normal(Square::B8, Square::C7), normal(Square::E1, Square::D1),
      normal(Square::C7, Square::D7));
}

TEST_F(MoveUndoTest, HandlesKingCaptureMove) {
  loadFen("1K1N4/R3b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP1Rr1N1/2k3Q1 b - - 0 1");
  testMoveSequence(normal(Square::C1, Square::D2), normal(Square::G2, Square::E1),
      normal(Square::A6, Square::A7), normal(Square::B8, Square::A7), normal(Square::E2, Square::F2),
      normal(Square::F3, Square::F4), normal(Square::D2, Square::E3));

  loadFen("3N4/R2Kb3/qP1Pn1pp/PP1PrBB1/p3p1b1/2pnPPpp/pP4Q1/3k4 w - - 4 6");
  testMoveSequence(normal(Square::D7, Square::E7), normal(Square::E5, Square::F5), normal(Square::E7, Square::E6));
}

TEST_F(MoveUndoTest, HandlesKingCastleMove) {
  loadFen("rnbqk2r/ppppppbp/5np1/8/3P4/5NP1/PPP1PPBP/RNBQK2R b KQkq - 2 4");
  testMoveSequence(castle(Square::E8, Square::G8), castle(Square::E1, Square::G1));

  loadFen("rnbqk2r/pp2ppbp/5np1/3p4/3P4/2N1P1P1/PP3PBP/R1BQK1NR b KQkq - 0 7");
  testMoveSequence(castle(Square::E8, Square::G8), normal(Square::G1, Square::E2),
      normal(Square::B8, Square::C6), castle(Square::E1, Square::G1));
}
