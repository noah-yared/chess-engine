#include <gtest/gtest.h>

#include <string>

#include "move.h"
#include "engine.h"
#include "test_utils.h"

class MoveApplyTest : public ::testing::Test {
protected:
  using Normal = Move<MoveType::Normal>;
  using DoublePush = Move<MoveType::DoublePawnPush>;
  using Enpassant = Move<MoveType::Enpassant>;
  using Promotion = Move<MoveType::Promotion>;
  using Castle = Move<MoveType::Castle>;

  Position pos;

  void loadStartingPosition() { pos = Position::fromStartingPosition(); }
  void loadFen(const std::string& fen) { pos = Position(fen); }
  void loadAscii(const std::string& ascii, const std::string& castlingRights = "KQkq", const std::string& side = "w",
      const std::string& enpassant = "-") { pos = Position::fromAscii(ascii, castlingRights, side, enpassant); }

  [[nodiscard]] Normal normal(Square from, Square to) const {
    if (auto capturedPiece = pos.getPieceOccupyingSquare(to); capturedPiece != PieceType::NONE)
      return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from), capturedPiece };
    return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from) };
  }
  [[nodiscard]] Promotion promotion(Square from, Square to) const {
    if (auto capturedPiece = pos.getPieceOccupyingSquare(to); capturedPiece != PieceType::NONE)
      return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from), capturedPiece };
    return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from) };
  }
  [[nodiscard]] Enpassant enpassant(Square from, Square to) const {
    return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from) };
  }
  [[nodiscard]] DoublePush doublePush(Square from, Square to) const {
    return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from) };
  }
  [[nodiscard]] Castle castle(Square from, Square to) const {
    return { from, to, pos.sideToMove(), pos.getPieceOccupyingSquare(from) };
  }

};

TEST_F(MoveApplyTest, HandlesPawnPushMove) {
  loadStartingPosition();
  pos.applyMove(normal(Square::A2, Square::A3));
  EXPECT_EQ(pos, Position("rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b KQkq - 0 1"));
  pos.applyMove(normal(Square::A7, Square::A6));
  EXPECT_EQ(pos, Position("rnbqkbnr/1ppppppp/p7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 0 2"));

  loadFen("3K4/Pp1p1N1n/bpPppk1P/PN2pp2/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 1");
  pos.applyMove(normal(Square::E5, Square::E4));
  EXPECT_EQ(pos, Position("3K4/Pp1p1N1n/bpPppk1P/PN3p2/1p1PpPBb/2RPB2q/3Pn3/1r1Q2Rr w - - 0 2"));
  pos.applyMove(normal(Square::D4, Square::D5));
  EXPECT_EQ(pos, Position("3K4/Pp1p1N1n/bpPppk1P/PN1P1p2/1p2pPBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 2"));

}

TEST_F(MoveApplyTest, HandlesPawnCaptureMove) {
  loadFen("3K4/Pp1p1N1n/bpPppk1P/PN2pp2/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 1");
  pos.applyMove(normal(Square::E5, Square::D4));
  EXPECT_EQ(pos, Position("3K4/Pp1p1N1n/bpPppk1P/PN3p2/1p1p1PBb/2RPB2q/3Pn3/1r1Q2Rr w - - 0 2"));
  pos.applyMove(normal(Square::C6, Square::B7));
  EXPECT_EQ(pos, Position("3K4/PP1p1N1n/bp1ppk1P/PN3p2/1p1p1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 2"));
  pos.applyMove(normal(Square::D4, Square::C3));
  EXPECT_EQ(pos, Position("3K4/PP1p1N1n/bp1ppk1P/PN3p2/1p3PBb/2pPB2q/3Pn3/1r1Q2Rr w - - 0 3"));
  pos.applyMove(normal(Square::D2, Square::C3));
  EXPECT_EQ(pos, Position("3K4/PP1p1N1n/bp1ppk1P/PN3p2/1p3PBb/2PPB2q/4n3/1r1Q2Rr b - - 0 3"));
}

TEST_F(MoveApplyTest, HandlesPawnDoublePushMove) {
  loadStartingPosition();
  pos.applyMove(doublePush(Square::A2, Square::A4));
  EXPECT_EQ(pos, Position("rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq a3 0 1"));
  pos.applyMove(doublePush(Square::D7, Square::D5));
  EXPECT_EQ(pos, Position("rnbqkbnr/ppp1pppp/8/3p4/P7/8/1PPPPPPP/RNBQKBNR w KQkq d6 0 2"));

  loadFen("1N2br1b/Kp2pR2/n1r1pp1N/P3Bnq1/3P1pp1/4P1Pp/1QP2RBk/8 w - - 1 6");
  pos.applyMove(doublePush(Square::C2, Square::C4));
  EXPECT_EQ(pos, Position("1N2br1b/Kp2pR2/n1r1pp1N/P3Bnq1/2PP1pp1/4P1Pp/1Q3RBk/8 b - c3 0 6"));
  pos.applyMove(doublePush(Square::B7, Square::B5));
  EXPECT_EQ(pos, Position("1N2br1b/K3pR2/n1r1pp1N/Pp2Bnq1/2PP1pp1/4P1Pp/1Q3RBk/8 w - b6 0 7"));
}

TEST_F(MoveApplyTest, HandlesPawnEnpassantMove) {
  loadFen("1N3r1b/K2bpR2/n1r1pp1N/Pp2Bnq1/3P1pp1/2P1P1Pp/2Q2RBk/8 w - b6 0 6");
  pos.applyMove(enpassant(Square::A5, Square::B6));
  EXPECT_EQ(pos, Position("1N3r1b/K2bpR2/nPr1pp1N/4Bnq1/3P1pp1/2P1P1Pp/2Q2RBk/8 b - - 0 6"));

  loadFen("1N3r1b/K2bpR2/n1r1pp2/P3BNq1/1pPP1pp1/4P1Pp/1Q3RBk/8 b - c3 0 7");
  pos.applyMove(enpassant(Square::B4, Square::C3));
  EXPECT_EQ(pos, Position("1N3r1b/K2bpR2/n1r1pp2/P3BNq1/3P1pp1/2p1P1Pp/1Q3RBk/8 w - - 0 8"));

  loadFen("1N3r1b/Kp1b4/n1rB1R1N/P2Ppnq1/5pp1/4P3/1QP2Rpk/8 w - e6 0 11");
  pos.applyMove(enpassant(Square::D5, Square::E6));
  EXPECT_EQ(pos, Position("1N3r1b/Kp1b4/n1rBPR1N/P4nq1/5pp1/4P3/1QP2Rpk/8 b - - 0 11"));
}

TEST_F(MoveApplyTest, HandlesPawnPromotionMove) {
  // Recall that engine defaults promotion piece to queen
  loadFen("1N3r1b/Kp1bP3/n1rB1R1N/P4nq1/5pp1/4P3/1QP2Rp1/7k b - - 0 12");
  pos.applyMove(promotion(Square::G2, Square::G1));
  EXPECT_EQ(pos, Position("1N3r1b/Kp1bP3/n1rB1R1N/P4nq1/5pp1/4P3/1QP2R2/6qk w - - 0 13"));
  pos.applyMove(promotion(Square::E7, Square::F8));
  EXPECT_EQ(pos, Position("1N3Q1b/Kp1b4/n1rB1R1N/P4nq1/5pp1/4P3/1QP2R2/6qk b - - 0 13"));

  loadFen("3K4/PP1p1N1n/bp1p1k1P/PN2pp2/1p1p1PBb/2RPB2q/3Pn3/1r1Q2Rr w - - 0 3");
  pos.applyMove(promotion(Square::B7, Square::B8));
  EXPECT_EQ(pos, Position("1Q1K4/P2p1N1n/bp1p1k1P/PN2pp2/1p1p1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 3"));
  pos.applyMove(normal(Square::B4, Square::C3));
  pos.applyMove(promotion(Square::A7, Square::A8));
  EXPECT_EQ(pos, Position("QQ1K4/3p1N1n/bp1p1k1P/PN2pp2/3p1PBb/2pPB2q/3Pn3/1r1Q2Rr b - - 0 4"));
}

TEST_F(MoveApplyTest, HandlesKnightCaptureMove) {
  loadFen("1N3Q1b/Kp1b4/n1rB1R1N/P4nq1/5pp1/4P3/1QP2R2/6qk b - - 0 13");
  pos.applyMove(normal(Square::F5, Square::E3));
  EXPECT_EQ(pos, Position("1N3Q1b/Kp1b4/n1rB1R1N/P5q1/5pp1/4n3/1QP2R2/6qk w - - 0 14"));
  pos.applyMove(normal(Square::H6, Square::G4));
  EXPECT_EQ(pos, Position("1N3Q1b/Kp1b4/n1rB1R2/P5q1/5pN1/4n3/1QP2R2/6qk b - - 0 14"));
  pos.applyMove(normal(Square::E3, Square::G4));
  EXPECT_EQ(pos, Position("1N3Q1b/Kp1b4/n1rB1R2/P5q1/5pn1/8/1QP2R2/6qk w - - 0 15"));

  loadFen("3K4/Pp1p1N2/bpPppk1P/PN2ppn1/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr w - - 1 2");
  pos.applyMove(normal(Square::F7, Square::G5));
  EXPECT_EQ(pos, Position("3K4/Pp1p4/bpPppk1P/PN2ppN1/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 2"));
  pos.applyMove(normal(Square::E2, Square::C3));
  EXPECT_EQ(pos, Position("3K4/Pp1p4/bpPppk1P/PN2ppN1/1p1P1PBb/2nPB2q/3P4/1r1Q2Rr w - - 0 3"));
}

TEST_F(MoveApplyTest, HandlesKnightNonCaptureMove) {
  loadStartingPosition();
  pos.applyMove(normal(Square::B1, Square::A3));
  EXPECT_EQ(pos, Position("rnbqkbnr/pppppppp/8/8/8/N7/PPPPPPPP/R1BQKBNR b KQkq - 1 1"));
  pos.applyMove(normal(Square::G8, Square::F6));
  EXPECT_EQ(pos, Position("rnbqkb1r/pppppppp/5n2/8/8/N7/PPPPPPPP/R1BQKBNR w KQkq - 2 2"));

  loadFen("3K4/Pp1p1N1n/bpPppk1P/PN2pp2/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 0 1");
  pos.applyMove(normal(Square::H7, Square::G5));
  EXPECT_EQ(pos, Position("3K4/Pp1p1N2/bpPppk1P/PN2ppn1/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr w - - 1 2"));
  pos.applyMove(normal(Square::B5, Square::C7));
  EXPECT_EQ(pos, Position("3K4/PpNp1N2/bpPppk1P/P3ppn1/1p1P1PBb/2RPB2q/3Pn3/1r1Q2Rr b - - 2 2"));
}

TEST_F(MoveApplyTest, HandlesBishopCaptureMove) {
  loadFen("1K6/Bp1p2P1/1qQpBnnp/p1NP4/1PpRppb1/1NP2r1R/PP3P1P/1k1rb3 w - - 1 2");
  pos.applyMove(normal(Square::E6, Square::D7));
  EXPECT_EQ(pos, Position("1K6/Bp1B2P1/1qQp1nnp/p1NP4/1PpRppb1/1NP2r1R/PP3P1P/1k1rb3 b - - 0 2"));
  pos.applyMove(normal(Square::G4, Square::H3));
  EXPECT_EQ(pos, Position("1K6/Bp1B2P1/1qQp1nnp/p1NP4/1PpRpp2/1NP2r1b/PP3P1P/1k1rb3 w - - 0 3"));
  pos.applyMove(normal(Square::D7, Square::H3));
  EXPECT_EQ(pos, Position("1K6/Bp4P1/1qQp1nnp/p1NP4/1PpRpp2/1NP2r1B/PP3P1P/1k1rb3 b - - 0 3"));
  pos.applyMove(normal(Square::E1, Square::C3));
  EXPECT_EQ(pos, Position("1K6/Bp4P1/1qQp1nnp/p1NP4/1PpRpp2/1Nb2r1B/PP3P1P/1k1r4 w - - 0 4"));
  pos.applyMove(normal(Square::A7, Square::B6));
  EXPECT_EQ(pos, Position("1K6/1p4P1/1BQp1nnp/p1NP4/1PpRpp2/1Nb2r1B/PP3P1P/1k1r4 b - - 0 4"));
}

TEST_F(MoveApplyTest, HandlesBishopNonCaptureMove) {
  loadFen("2R4K/R1P1p1p1/1P1B3n/bB3P2/brppP3/nP1p1P1P/k3ppPp/Nq2Q1rN w - - 0 1");
  pos.applyMove(normal(Square::B5, Square::D7));
  EXPECT_EQ(pos, Position("2R4K/R1PBp1p1/1P1B3n/b4P2/brppP3/nP1p1P1P/k3ppPp/Nq2Q1rN b - - 1 1"));
  pos.applyMove(normal(Square::A4, Square::C6));
  EXPECT_EQ(pos, Position("2R4K/R1PBp1p1/1PbB3n/b4P2/1rppP3/nP1p1P1P/k3ppPp/Nq2Q1rN w - - 2 2"));
  pos.applyMove(normal(Square::D6, Square::F4));
  EXPECT_EQ(pos, Position("2R4K/R1PBp1p1/1Pb4n/b4P2/1rppPB2/nP1p1P1P/k3ppPp/Nq2Q1rN b - - 3 2"));
  pos.applyMove(normal(Square::C6, Square::A8));
}

TEST_F(MoveApplyTest, HandlesRookCaptureMove) {
  loadFen("b1R4K/R1PBp1p1/1P5n/b4P2/1rppPB2/nP1p1P1P/k3ppPp/Nq2Q1rN w - - 4 3");
  pos.applyMove(normal(Square::C8, Square::A8));
  EXPECT_EQ(pos, Position("R6K/R1PBp1p1/1P5n/b4P2/1rppPB2/nP1p1P1P/k3ppPp/Nq2Q1rN b - - 0 3"));
  pos.applyMove(normal(Square::G1, Square::E1));
  EXPECT_EQ(pos, Position("R6K/R1PBp1p1/1P5n/b4P2/1rppPB2/nP1p1P1P/k3ppPp/Nq2r2N w - - 0 4"));
  pos.applyMove(normal(Square::A7, Square::A5));
  EXPECT_EQ(pos, Position("R6K/2PBp1p1/1P5n/R4P2/1rppPB2/nP1p1P1P/k3ppPp/Nq2r2N b - - 0 4"));
}

TEST_F(MoveApplyTest, HandlesRookNonCaptureMove) {
  loadFen("1rbbN3/Kpp2p1Q/1n4nP/BP1BPp2/k3ppPp/3PN2p/2RR1PP1/1q5r w - - 0 2");
  pos.applyMove(normal(Square::C2, Square::C6));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2p1Q/1nR3nP/BP1BPp2/k3ppPp/3PN2p/3R1PP1/1q5r b - - 1 2"));
  pos.applyMove(normal(Square::H1, Square::C1));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2p1Q/1nR3nP/BP1BPp2/k3ppPp/3PN2p/3R1PP1/1qr5 w - - 2 3"));
  pos.applyMove(normal(Square::D2, Square::A2));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2p1Q/1nR3nP/BP1BPp2/k3ppPp/3PN2p/R4PP1/1qr5 b - - 3 3"));
}

TEST_F(MoveApplyTest, HandlesQueenCaptureMove) {
  loadFen("1rbbN3/Kpp2p1Q/1nR3nP/BP1BPp2/k3ppPp/3PN2p/R4PP1/1qr5 b - - 3 3");
  pos.applyMove(normal(Square::B1, Square::A2));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2p1Q/1nR3nP/BP1BPp2/k3ppPp/3PN2p/q4PP1/2r5 w - - 0 4"));
  pos.applyMove(normal(Square::H7, Square::G6));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2p2/1nR3QP/BP1BPp2/k3ppPp/3PN2p/q4PP1/2r5 b - - 0 4"));
  pos.applyMove(normal(Square::A2, Square::D5));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2p2/1nR3QP/BP1qPp2/k3ppPp/3PN2p/5PP1/2r5 w - - 0 5"));
  pos.applyMove(normal(Square::G6, Square::F7));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2Q2/1nR4P/BP1qPp2/k3ppPp/3PN2p/5PP1/2r5 b - - 0 5"));
  pos.applyMove(normal(Square::D5, Square::F7));
  EXPECT_EQ(pos, Position("1rbbN3/Kpp2q2/1nR4P/BP2Pp2/k3ppPp/3PN2p/5PP1/2r5 w - - 0 6"));
}

TEST_F(MoveApplyTest, HandlesQueenNonCaptureMove) {
  loadFen("2qb3R/rn3kPB/pR3B1p/P2P2bK/2p1P3/1P1Pp1pP/1NrppPp1/3N2Q1 b - - 0 1");
  pos.applyMove(normal(Square::C8, Square::F5));
  EXPECT_EQ(pos, Position("3b3R/rn3kPB/pR3B1p/P2P1qbK/2p1P3/1P1Pp1pP/1NrppPp1/3N2Q1 w - - 1 2"));
  pos.applyMove(normal(Square::G1, Square::E1));
  EXPECT_EQ(pos, Position("3b3R/rn3kPB/pR3B1p/P2P1qbK/2p1P3/1P1Pp1pP/1NrppPp1/3NQ3 b - - 2 2"));
  pos.applyMove(normal(Square::F5, Square::E5));
  EXPECT_EQ(pos, Position("3b3R/rn3kPB/pR3B1p/P2Pq1bK/2p1P3/1P1Pp1pP/1NrppPp1/3NQ3 w - - 3 3"));
}

TEST_F(MoveApplyTest, HandlesKingNormalMove) {
  loadFen("1K1N4/R3b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP4Q1/4k3 w - - 0 4");
  pos.applyMove(normal(Square::B8, Square::C7));
  EXPECT_EQ(pos, Position("3N4/R1K1b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP4Q1/4k3 b - - 1 4"));
  pos.applyMove(normal(Square::E1, Square::D1));
  EXPECT_EQ(pos, Position("3N4/R1K1b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP4Q1/3k4 w - - 2 5"));
  pos.applyMove(normal(Square::C7, Square::D7));
  EXPECT_EQ(pos, Position("3N4/R2Kb3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP4Q1/3k4 b - - 3 5"));
}

TEST_F(MoveApplyTest, HandlesKingCaptureMove) {
  loadFen("1K1N4/R3b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP1Rr1N1/2k3Q1 b - - 0 1");
  pos.applyMove(normal(Square::C1, Square::D2));
  EXPECT_EQ(pos, Position("1K1N4/R3b3/qP1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP1kr1N1/6Q1 w - - 0 2"));
  pos.applyMove(normal(Square::G2, Square::E1));
  pos.applyMove(normal(Square::A6, Square::A7));
  pos.applyMove(normal(Square::B8, Square::A7));
  EXPECT_EQ(pos, Position("3N4/K3b3/1P1P2pp/PP1PrBB1/p2np1b1/2pnPPpp/pP1kr3/4N1Q1 b - - 0 3"));
  pos.applyMove(normal(Square::E2, Square::F2));
  pos.applyMove(normal(Square::F3, Square::F4));
  pos.applyMove(normal(Square::D2, Square::E3));
  EXPECT_EQ(pos, Position("3N4/K3b3/1P1P2pp/PP1PrBB1/p2npPb1/2pnk1pp/pP3r2/4N1Q1 w - - 0 5"));

  loadFen("3N4/R2Kb3/qP1Pn1pp/PP1PrBB1/p3p1b1/2pnPPpp/pP4Q1/3k4 w - - 4 6");
  pos.applyMove(normal(Square::D7, Square::E7));
  EXPECT_EQ(pos, Position("3N4/R3K3/qP1Pn1pp/PP1PrBB1/p3p1b1/2pnPPpp/pP4Q1/3k4 b - - 0 6"));
  pos.applyMove(normal(Square::E5, Square::F5));
  pos.applyMove(normal(Square::E7, Square::E6));
  EXPECT_EQ(pos, Position("3N4/R7/qP1PK1pp/PP1P1rB1/p3p1b1/2pnPPpp/pP4Q1/3k4 b - - 0 7"));
}

TEST_F(MoveApplyTest, HandlesKingCastleMove) {
  loadFen("rnbqk2r/ppppppbp/5np1/8/3P4/5NP1/PPP1PPBP/RNBQK2R b KQkq - 2 4");
  pos.applyMove(castle(Square::E8, Square::G8));
  EXPECT_EQ(pos, Position("rnbq1rk1/ppppppbp/5np1/8/3P4/5NP1/PPP1PPBP/RNBQK2R w KQ - 3 5"));
  pos.applyMove(castle(Square::E1, Square::G1));
  EXPECT_EQ(pos, Position("rnbq1rk1/ppppppbp/5np1/8/3P4/5NP1/PPP1PPBP/RNBQ1RK1 b - - 4 5"));

  loadFen("rnbqk2r/pp2ppbp/5np1/3p4/3P4/2N1P1P1/PP3PBP/R1BQK1NR b KQkq - 0 7");
  pos.applyMove(castle(Square::E8, Square::G8));
  EXPECT_EQ(pos, Position("rnbq1rk1/pp2ppbp/5np1/3p4/3P4/2N1P1P1/PP3PBP/R1BQK1NR w KQ - 1 8"));
  pos.applyMove(normal(Square::G1, Square::E2));
  pos.applyMove(normal(Square::B8, Square::C6));
  pos.applyMove(castle(Square::E1, Square::G1));
  EXPECT_EQ(pos, Position("r1bq1rk1/pp2ppbp/2n2np1/3p4/3P4/2N1P1P1/PP2NPBP/R1BQ1RK1 b - - 4 9"));
}
