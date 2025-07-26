#include <gtest/gtest.h>

#include "engine.h"
#include "position.h"
#include "test_utils.h"

TEST(CheckDetectionTest, HandlesUnthreatenedKing) {
  // no pins, no checks
  Position pos1("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos1.isKingInCheck(Color::BLACK));

  Position pos2("2b5/4Bpbp/7r/p1Np4/2p2P1P/5P1p/1k1P4/1B3R1K w - - 0 13");
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos2.isKingInCheck(Color::BLACK));

  Position pos3("2b5/p2NBpPp/4nP1r/3p4/2pb1r1P/1k1B1PPp/1P1P4/5R1K b - - 0 6");
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos3.isKingInCheck(Color::BLACK));

  Position pos4("2R5/rn3kbB/p6p/P2P2bK/2p1P3/1PBPp1pP/1NrppPp1/3N2Q1 w - - 0 3");
  EXPECT_FALSE(pos4.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos4.isKingInCheck(Color::BLACK));

  Position pos5("3K4/kp2prp1/2BpQ1P1/1qpp1NP1/Pb4P1/1ppP1b2/1BPP1nrP/1N2R2R b - - 0 1");
  EXPECT_FALSE(pos5.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos5.isKingInCheck(Color::BLACK));
}

TEST(CheckDetectionTest, HandlesRookPin) {
  Position pos1("3b2Q1/rkq4R/p2B1b1p/P2P1B1K/2p1P3/1P1Pp1pP/1NrppPp1/3N2Q1 w - - 7 8");
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos1.isKingInCheck(Color::BLACK));

  Position pos2("2R4K/2P1p1p1/1P5n/1B3P2/3pP3/RPpp1P1P/q2B2Pp/k4qrN w - - 1 7");
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos2.isKingInCheck(Color::BLACK));

  Position pos3("2R4K/R1P1p1p1/1P1B3n/b4P2/2ppP3/3p1P1P/k1B1ppPp/Nq2Q1rN b - - 0 3");
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos3.isKingInCheck(Color::BLACK));
}

TEST(CheckDetectionTest, HandlesBishopPin) {
  Position pos1("2qb2QR/r3k2B/p4b1p/P1nP3K/1Bp1P3/1P1Pp1pP/1NrppPp1/3N2Q1 w - - 3 4");
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos1.isKingInCheck(Color::BLACK));

  Position pos2("2qR1r2/1p2P1p1/1Pn3R1/p1b1kr1B/1PK3pp/P1n1PpPP/P3Q1N1/B2b1N1b w - - 0 4");
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos2.isKingInCheck(Color::BLACK));

  Position pos3("2qR1r2/1p2P1p1/1Pn1krR1/p1P4B/6pp/PKn1PpPP/P1Q3N1/B2b1N1b b - - 0 6");
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos3.isKingInCheck(Color::BLACK));
}

TEST(CheckDetectionTest, HandlesQueenPin) {
  Position pos1("2qb3R/rn1kbQ1B/p6p/P2P3K/1Bp1P3/1P1Pp1pP/1NrppPp1/3N2Q1 w - - 5 5");
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos1.isKingInCheck(Color::BLACK));

  Position pos2("2R5/rn5B/p4k1p/P2P2bK/2p1P3/1P1PpPpP/1Nrp2Q1/3q4 b - - 0 5");
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos2.isKingInCheck(Color::BLACK));

  Position pos3("k2b4/1rq4R/p2P1b1p/P1BQ1B1K/2p1P2P/1P1Pp1p1/1NrppPp1/3N2Q1 w - - 3 12");
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos3.isKingInCheck(Color::BLACK));
}

TEST(CheckDetectionTest, HandlesPawnCheck) {
  Position pos1("2qR1r2/1p2P1p1/1Pn3R1/p3kr1B/3b2pp/PP1pPpPP/P1KpQ1N1/Bn3N1b w - - 0 1");
  EXPECT_TRUE(pos1.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos1.isKingInCheck(Color::BLACK));

  Position pos2("2qR1r2/1p2P1p1/1Pn1krR1/2P4B/p5pp/PKn1PpPP/P1Q3N1/B2b1N1b w - - 0 7");
  EXPECT_TRUE(pos2.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos2.isKingInCheck(Color::BLACK));

  Position pos3("8/2p1kpqb/R2RnP1p/n2P3P/1N1ppQ1K/rPP1rp2/p1P1P3/2B2NbB b - - 0 2");
  EXPECT_TRUE(pos3.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
}

TEST(CheckDetectionTest, HandlesKnightCheck) {
  Position pos1("2b5/p3Bpnp/5P1r/3p4/N1pb1r1P/3B1PPp/1k1P4/5R1K b - - 1 8");
  EXPECT_TRUE(pos1.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));

  Position pos2("2b5/p3Bpnp/5P1r/2Np4/2pb1r1P/1k1B1PPp/1P1P4/5R1K b - - 1 7");
  EXPECT_TRUE(pos2.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));

  Position pos3("2b5/p2NBp1p/1b2nPPr/3p4/2pR1r1P/1k1B1Pnp/1P1P3P/5R1K w - - 0 5");
  EXPECT_TRUE(pos3.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos3.isKingInCheck(Color::BLACK));
}

TEST(CheckDetectionTest, HandlesBishopCheck) {
  Position pos1("2b5/p3Bpbp/7r/3p4/N1p2r1P/5PPp/k2P4/1B3R1K b - - 1 10");
  EXPECT_TRUE(pos1.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));

  Position pos2("2q4R/rn1kb3/p6p/P2P1B1K/1Bp1P3/1P1Pp1pP/1NrppPp1/3N2Q1 b - - 1 6");
  EXPECT_TRUE(pos2.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));

  Position pos3("2qb2QR/rn2k2B/p4b1p/P2P3K/1Bp1P3/1P1Pp1pP/1NrppPp1/3N2Q1 b - - 2 3");
  EXPECT_TRUE(pos3.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
}

TEST(CheckDetectionTest, HandlesQueenCheck) {
  Position pos1("2b5/p2NBp1p/1bp1nPPr/3P4/2pRnr1P/1k1B1Ppp/1P1P1pQP/R2q3K w - - 0 2");
  EXPECT_TRUE(pos1.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos1.isKingInCheck(Color::BLACK));

  Position pos2("2b5/p2NBp1p/1bp1nPPr/3P4/2pRnr1P/1k1B1Ppp/1P1P2QP/3R1q1K w - - 0 3");
  EXPECT_TRUE(pos2.isKingInCheck(Color::WHITE));
  EXPECT_FALSE(pos2.isKingInCheck(Color::BLACK));

  Position pos3("2qb3R/rn1k1Q1B/p4b1p/P2P3K/1Bp1P3/1P1Pp1pP/1NrppPp1/3N2Q1 b - - 4 4");
  EXPECT_TRUE(pos3.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
}

TEST(CheckDetectionTest, HandlesRookCheck) {
  Position pos1("2qb2Q1/rk5R/p2B1b1p/P2P1B1K/2p1P3/1P1Pp1pP/1NrppPp1/3N2Q1 b - - 6 7");
  EXPECT_TRUE(pos1.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos1.isKingInCheck(Color::WHITE));

  Position pos2("2R4K/2P1p1p1/1P5n/1B3P2/3pP3/RPpp1P1P/3B2Pp/kq3qrN b - - 0 6");
  EXPECT_TRUE(pos2.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos2.isKingInCheck(Color::WHITE));

  Position pos3("R4k2/2p2pqb/3RnP1p/n2P3P/1N1ppQ1K/rPP1rp2/p1P1P3/2B2NbB b - - 2 3");
  EXPECT_TRUE(pos3.isKingInCheck(Color::BLACK));
  EXPECT_FALSE(pos3.isKingInCheck(Color::WHITE));
}
