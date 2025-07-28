#pragma once

#include <algorithm>
#include <cassert>

#include <concepts>
#include <optional>
#include <random>
#include <type_traits>
#include <variant>

#include "board_state.h"
#include "board_state_snapshot.h"
#include "bitboards.h"
#include "bit_utils.h"
#include "constants.h"
#include "directions.h"
#include "evaluate.h"
#include "move.h"
#include "move_list.h"
#include "piece_square_deltas.h"
#include "precomputed_attacks.h"
#include "state_update_helpers.h"
#include "zobrist_hasher.h"


class Position {
public:
  using RNG = std::mt19937_64; // random number generator for hashing function

  friend std::ostream& operator<<(std::ostream& os, const Position& pos);

  /////////////////////////
  // Constructors        //
  /////////////////////////
  Position() : bitboards_{}, state_{}, hash_{ZobristHasher<RNG>::initialZobristHash()}, moves_{} {};
  explicit Position(const std::string& fen) : bitboards_{fen, FromFEN{}}, state_{fen},
      hash_{ZobristHasher<RNG>::computeZobristHash(bitboards_, state_)}, moves_{} {};
  
  /////////////////////////
  // Factory Methods     //
  /////////////////////////
  static Position fromStartingPosition() {
    return Position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  }
  
  static Position fromAscii(const std::string& asciiBoard, const std::string turn = "w",
      const std::string castlingRights = "-", const std::string enpassant = "-") {
    return {
      Bitboards(asciiBoard, FromAsciiBoard{}),
      BoardState(turn, castlingRights, enpassant)
    };
  }

  /////////////////////////
  // Position Loading    //
  /////////////////////////
  void loadFEN(const std::string& fen) {
    bitboards_ = Bitboards(fen, FromFEN{});
    state_ = BoardState(fen);
    hash_ = ZobristHasher<RNG>::computeZobristHash(bitboards_, state_);
    moves_.clear();
  }
  
  void loadAsciiBoard(const std::string& asciiBoard, const std::string turn = "w",
      const std::string castlingRights = "-", const std::string enpassant = "-") {
    bitboards_ = Bitboards(asciiBoard, FromAsciiBoard{});
    state_ = BoardState(turn, castlingRights, enpassant);
    hash_ = ZobristHasher<RNG>::computeZobristHash(bitboards_, state_);
    moves_.clear();
  }

  /////////////////////////
  // Move Operations     //
  /////////////////////////
  template<MoveType mType>
  void applyMove(const Move<mType> move) {
    const auto deltas = pieceSquareDeltas<mType>(move);
    updateBitboards<mType>(move, deltas);
    auto extractedPrevState = state_.extract();
    updateState<mType>(move);
    updateHash<mType>(move, deltas, BoardState(extractedPrevState), state_);
  }

  template<MoveType mType>
  void undoMove(const Move<mType> move, const BoardStateSnapshot previousSnapshot) {
    revertBitboards(move);
    auto [prevState, prevHash] = previousSnapshot;
    state_.revert(prevState), hash_ = prevHash;
  }

  template<MoveType mType>
  bool pushIfSafe(Move<mType> candidateMove) const {
    if (doesMoveExposeAllyKing(candidateMove)) return false;
    return moves_.push(candidateMove), true;
  }

  const MoveList& legalMoves() const;

  /////////////////////////
  // Move Validation     //
  /////////////////////////
  bool isKingInCheck(Color color) const {
    return ! isBitboardSafeFromSide( bitboards_.bb(PieceType::KING, color), opposite(color) );
  }

  template<MoveType mType>
  bool doesMoveExposeAllyKing(const Move<mType> move) const {
    Position tmp = *this;
    tmp.applyMove(move);
    return tmp.isKingInCheck(move.side());
  }

  /////////////////////////
  // State Interface     //
  /////////////////////////
  [[nodiscard]] BoardStateSnapshot getStateSnapshot() const { return { .state = state_.extract(), .hash = hash_ }; }
  [[nodiscard]] u64 getHash() const { return hash_; } 
  [[nodiscard]] int evaluation() const { return Evaluator::evaluate(bitboards_); }
  [[nodiscard]] std::optional<int> maybeEnpassantSquare() const { return state_.getEnpassantSquare(); }
  [[nodiscard]] int castlingRights() const { return state_.castlingBits(); }
  [[nodiscard]] std::vector<int> availableCastlingDests(Color color) const { return state_.availableCastlingDestinations(color); }
  [[nodiscard]] Color sideToMove() const { return state_.getTurn(); }
  [[nodiscard]] bool isWhiteToMove() const { return state_.getTurn() == Color::WHITE; }

  /////////////////////////
  // Bitboards Interface //
  /////////////////////////
  // King square
  [[nodiscard]] int kingSquare(Color color) const { return bitboards_.king(color); }
  
  // Piece occupancy queries
  [[nodiscard]] u64 getPieceBitboard(PieceType pType, Color color) const { return bitboards_.bb(pType, color); }
  [[nodiscard]] PieceType getPieceOccupyingSquare(int square) const { return bitboards_.getPieceType(square); }
  [[nodiscard]] PieceType getPieceOccupyingSquare(int square, Color color) const { return bitboards_.getPieceType(square, color); }
  [[nodiscard]] bool isPieceOccupyingSquare(PieceType pType, Color color, int square) const { return bitboards_.bb(pType, color) & 1ULL << square; }
  [[nodiscard]] bool isAllyOccupyingSquare(int square, Color color) const { return bitboards_.allyBB(color) & 1ULL << square; }
  [[nodiscard]] bool isEnemyOccupyingSquare(int square, Color color) const { return bitboards_.opposingBB(color) & 1ULL << square; }
  [[nodiscard]] bool isSquareOccupied(int square) const { return bitboards_.combinedBB() & 1ULL << square; }

  // Bitboard filters
  [[nodiscard]] u64 filterOccupiedSquares(u64 bb) const { return bb & bitboards_.combinedBB(); }
  [[nodiscard]] u64 filterAllySquares(u64 bb, Color color) const { return bb & bitboards_.allyBB(color); }
  [[nodiscard]] u64 filterEnemySquares(u64 bb, Color color) const { return bb & bitboards_.opposingBB(color); }
  template<int rank>
  [[nodiscard]] static u64 filterRankFromBitboard(u64 bb) { return BitUtils::filterRank<rank>(bb); }
  template<char file>
  [[nodiscard]] static u64 filterFileFromBitboard(u64 bb) { return BitUtils::filterFile<file>(bb); }

  // Bitboard clearers
  [[nodiscard]] u64 clearOccupiedSquares(u64 bb, Color color) const { return bb & ~bitboards_.combinedBB(); }
  [[nodiscard]] u64 clearAllySquares(u64 bb, Color color) const { return bb & ~bitboards_.allyBB(color); }
  [[nodiscard]] u64 clearEnemySquares(u64 bb, Color color) const { return bb & ~bitboards_.opposingBB(color); }
  template<int rank>
  [[nodiscard]] static u64 clearRankFromBitboard(u64 bb) { return BitUtils::clearRank<rank>(bb); }
  template<char file>
  [[nodiscard]] static u64 clearFileFromBitboard(u64 bb) { return BitUtils::clearFile<file>(bb); }

  /////////////////////////
  // Utility Methods     //
  /////////////////////////
  void clearMoveBuffer() const { moves_.clear(); }
  std::string stringifyBitboards() const { return bitboards_.toString(); }
  std::string stringifyBoardState() const { return state_.toString(); }
  bool areBitboardsConsistent() const { return bitboards_.isConsistent(); }

  // Useful edge detection helpers
  [[nodiscard]] static bool isSquareOnLeftEdge(int square) { return (square ^ 7) == 0; }
  [[nodiscard]] static bool isSquareOnRightEdge(int square) { return (square ^ 7) == 7; }

  /////////////////////////
  // Operators           //
  /////////////////////////
  bool operator==(const Position& other) const { return bitboards_ == other.bitboards_ && state_ == other.state_ && hash_ == other.hash_; } 
  bool operator!=(const Position& other) const { return ! (operator==(other)); }

private:
  Bitboards bitboards_;
  BoardState state_;
  u64 hash_;
  mutable MoveList moves_;

  /////////////////////////
  // Private Constructor //
  /////////////////////////
  Position(Bitboards bitboards, BoardState state) :
      bitboards_{bitboards},
      state_{state},
      hash_{ZobristHasher<RNG>::computeZobristHash(bitboards, state)},
      moves_{} {};

  /////////////////////////
  // Move Generation     //
  /////////////////////////
  template<MoveType mType, PieceType pType>
  void pushLegalMoves() const {
    static_assert(mType == MoveType::Normal && pType != PieceType::PAWN,
        "Pawn and Castle moves should be handled separately in specializations");
    BitUtils::bitsForEach<>(
        getPieceBitboard(pType, sideToMove()), [&](int start) {
          BitUtils::bitsForEach<>(clearAllySquares(attackedSquares<pType>(start, sideToMove()), sideToMove()),
              [&](int dest) { pushIfSafe(MoveFactory::createMove<mType, pType>(start, dest, *this)); });
        });
  }

  /////////////////////////
  // Move Factory        //
  /////////////////////////
  class MoveFactory {
  public:
    template<MoveType mType, PieceType pType>
    static Move<mType> createMove(int start, int end, const Position& pos) {   
      auto capturedPiece = pos.getPieceOccupyingSquare(end, opposite(pos.sideToMove()));
      if (capturedPiece != PieceType::NONE) {
        return Move<mType>(start, end, pos.sideToMove(), pType, capturedPiece);
      } else {
        return Move<mType>(start, end, pos.sideToMove(), pType);
      }
    }

    // partial template specialization for non capturing move types (enpassant, double pawn push, castle)
    // as they can not capture pieces, avoiding an unnecessary branch that checks for captured piece (always false)
    template<MoveType mType, PieceType pType>
    requires (! CanCapture<mType>)
    static Move<mType> createMove(int start, int end, const Position& pos) {
      return Move<mType>(start, end, pos.sideToMove(), pType);
    }
  };

  /////////////////////////
  // State Updates       //
  /////////////////////////
  template<MoveType mType>
  void updateBitboards(const Move<mType> move, const std::vector<Delta>& deltas) {
    for (const auto [key, square] : deltas)
      bitboards_.togglePieceSquare(key, square);
  }
  
  template<MoveType mType>
  void revertBitboards(const Move<mType> move) {
    for (const auto [key, square] : pieceSquareDeltas<mType>(move))
      bitboards_.togglePieceSquare(key, square);
  }

  template<MoveType mType>
  void updateState(const Move<mType> move) {
    updateEnpassantSquare<mType>(move, state_);
    updateCastlingPrivs<mType>(move, state_);
    updateTurn<mType>(move, state_);
  }

  template<MoveType mType>
  void updateHash(const Move<mType> move, const std::vector<Delta>& deltas, const BoardState prevState,
      const BoardState newState) {
    hash_ ^= ZobristHasher<RNG>::getHashUpdateMask<mType>(move, deltas, prevState.getEnpassantSquare(),
        prevState.castlingBits(), newState.castlingBits());
  }

  /////////////////////////
  // Safety Checks       //
  /////////////////////////
  // only check valid castle when making move so assume color is the side to move
  bool isCastleSafe(int dest) const {
    int king = kingSquare(sideToMove());
    u64 emptySquaresMask = king < dest
        ? QUEENSIDE_CASTLE_MASK(sideToMove())
        :  KINGSIDE_CASTLE_MASK(sideToMove());
    if (filterOccupiedSquares(emptySquaresMask)) // piece in between the rook and king so no castle 
      return false;
    // Create bitboard mask for all squares that need to be safe
    u64 safetyMask = (1ULL << king) | (1ULL << ((king + dest) / 2)) | (1ULL << dest);
    return isBitboardSafeFromSide(safetyMask, opposite(sideToMove()));
  }

  bool isBitboardSafeFromSide(u64 bb, Color color) const {
    return ! isBitboardAttackedBySide<PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP,
        PieceType::ROOK, PieceType::QUEEN,  PieceType::KING>(bb, color);
  }

  bool isSquareSafeFromSide(int square, Color color) const {
    return isBitboardSafeFromSide(1ULL << square, color);
  }

  template<PieceType pType, PieceType ...rest>
  bool isBitboardAttackedBySide(u64 bb, Color color) const {
    if constexpr(sizeof...(rest) > 0)
      return (bb & controlledSquares<pType>(color)) || isBitboardAttackedBySide<rest...>(bb, color);
    return (bb & controlledSquares<pType>(color));
  }

  /////////////////////////
  // Attack Generation   //
  /////////////////////////
  template<PieceType pType, PieceType ...rest>
  u64 controlledSquares(Color color) const {
    u64 ctrlSquares = rawControlledSquares<pType>(color);
    if constexpr(sizeof...(rest) > 0)
      ctrlSquares |= controlledSquares<rest...>(color);
    return ctrlSquares;
  }

  // Raw controlled squares (all squares piece can attack, including allies)
  template<PieceType pType>
  u64 rawControlledSquares(Color color) const {
    return BitUtils::accumulateBits<u64>(getPieceBitboard(pType, color), [&](u64 targets, int lsb) {
      return targets | attackedSquares<pType>(lsb, color);
    });
  }

  // Specializations for rawControlledSquares
  template<>
  inline u64 rawControlledSquares<PieceType::PAWN>(Color color) const {
    return squaresAttackedByPawns(color);
  }

  inline u64 singlePawnPushTargets(Color color) const {
    auto pushDests = [&]<Color color> {
      return clearOccupiedSquares(Position::stepBitboard<forward<color>>(getPieceBitboard(PieceType::PAWN, color)), color);
    };
    return color == Color::WHITE ? pushDests.operator()<Color::WHITE>() : pushDests.operator()<Color::BLACK>();
  }

  inline u64 squaresAttackedByPawns(Color color) const {
    auto getAttackedSquares = [&]<Color color>() -> u64 {
      return Position::stepBitboard<leftPawnAttack<color>, rightPawnAttack<color>>(
          getPieceBitboard(PieceType::PAWN, color),
          [](u64 bb) { return Position::clearFileFromBitboard<'a'>(bb); },
          [](u64 bb) { return Position::clearFileFromBitboard<'h'>(bb); });
    };
    return color == Color::WHITE ? getAttackedSquares.operator()<Color::WHITE>() : getAttackedSquares.operator()<Color::BLACK>();
  }

  /////////////////////////
  // Sliding Attacks     //
  /////////////////////////
  [[nodiscard]] u64 attackedDiagonalSquares(int square, Color color) const {
    return attackedSquaresAlongLaneFromSquare<Direction::NW>(square, color) |
           attackedSquaresAlongLaneFromSquare<Direction::NE>(square, color) |
           attackedSquaresAlongLaneFromSquare<Direction::SE>(square, color) |
           attackedSquaresAlongLaneFromSquare<Direction::SW>(square, color);
  }
  
  [[nodiscard]] u64 attackedOrthogonalSquares(int square, Color color) const {
    return attackedSquaresAlongLaneFromSquare<Direction::N>(square, color) |
           attackedSquaresAlongLaneFromSquare<Direction::E>(square, color) |
           attackedSquaresAlongLaneFromSquare<Direction::S>(square, color) |
           attackedSquaresAlongLaneFromSquare<Direction::W>(square, color);
  }
  
  template<PieceType pType>
  [[nodiscard]] u64 attackedSquares(int square, Color color) const {
    static_assert(pType != PieceType::PAWN, "Pawns are not supported in this function; use this for other piece types");
    if constexpr (pType == PieceType::KNIGHT)
      return Attacks::getKnightAttackBitmap(square);
    if constexpr (pType == PieceType::KING)
      return Attacks::getKingAttackBitmap(square);
    if constexpr (pType == PieceType::BISHOP)
      return attackedDiagonalSquares(square, color);
    if constexpr (pType == PieceType::ROOK)
      return attackedOrthogonalSquares(square, color);
    return attackedDiagonalSquares(square, color) | attackedOrthogonalSquares(square, color); // queen
  }

  // sliding attack helpers
  template<Direction upwardLane>
  requires (Directions::isUpwards(upwardLane))
  u64 attackedSquaresAlongLaneFromSquare(int square, Color color) const {
    u64 attackRay = Attacks::getSlidingAttackBitmap(square, upwardLane);
    u64 piecesOnLane = filterOccupiedSquares(attackRay);
    if (!piecesOnLane) // no blockers - free lane
      return attackRay;
    return attackRay & ~Attacks::getSlidingAttackBitmap(BitUtils::ctz(piecesOnLane), upwardLane);
  }

  template<Direction downwardLane>
  requires (! Directions::isUpwards(downwardLane))
  u64 attackedSquaresAlongLaneFromSquare(int square, Color color) const {
    u64 attackRay = Attacks::getSlidingAttackBitmap(square, downwardLane);
    u64 piecesOnLane = filterOccupiedSquares(attackRay);
    if (!piecesOnLane) // no blockers - free lane
      return attackRay;
    return attackRay & ~Attacks::getSlidingAttackBitmap(63 ^ BitUtils::clz(piecesOnLane), downwardLane);
  }

  /////////////////////////
  // Bitboard Utilities  //
  /////////////////////////
  // useful default unary identity function (bitboard => bitboard) for templates
  constexpr static inline auto identity = [](u64 bb) { return bb; };
  
  // bitboard shift helpers 
  template<Direction direction, Direction ...directions, typename UnaryOp=decltype(Position::identity), typename... UnaryOps>
  requires (std::is_invocable_v<UnaryOp, u64> &&
      requires(u64 bb, UnaryOp f){ { f(bb) } -> std::same_as<u64>; })
  [[nodiscard]] static u64 stepBitboard(u64 bb, UnaryOp func=identity, UnaryOps... funcs) {
    u64 steppedBB = BitUtils::stepBitsForward(func(bb), direction);
    if constexpr (sizeof...(directions) > 0)
      return steppedBB | stepBitboard<directions...>(bb, funcs...);
    return steppedBB;
  }

  /////////////////////////
  // Pawn Move Helpers   //
  /////////////////////////
  template<Color color>
  void pushPawnAttackMoves(int dest) const {
    int leftAtkSquare = dest - Directions::sfamt(leftPawnAttack<color>);
    int rightAtkSquare = dest - Directions::sfamt(rightPawnAttack<color>);
    if (!isSquareOnLeftEdge(dest) && isPieceOccupyingSquare(PieceType::PAWN, color, leftAtkSquare))
      pushIfSafe(MoveFactory::createMove<MoveType::Normal, PieceType::PAWN>(leftAtkSquare, dest, *this));
    if (!isSquareOnRightEdge(dest) && isPieceOccupyingSquare(PieceType::PAWN, color, rightAtkSquare))
      pushIfSafe(MoveFactory::createMove<MoveType::Normal, PieceType::PAWN>(rightAtkSquare, dest, *this));
  }
};

inline std::ostream& operator<<(std::ostream& os, const Position& pos) {
  os << "Position:\n"
     << pos.stringifyBoardState() << '\n'
     << std::hex << pos.getHash() << std::dec << '\n'
     << pos.stringifyBitboards() << '\n';
  return os;
}

///////////////////////////////
// Template Specializations  //
///////////////////////////////

// Position::doesMoveExposeAllyKing() specialization
template<>
inline bool Position::doesMoveExposeAllyKing<MoveType::Castle>(const Move<MoveType::Castle> move) const {
  return Position::isCastleSafe(move.end());
}

// Position::pushLegalMoves() specializations for move types not handled in primary template
template<>
inline void Position::pushLegalMoves<MoveType::Normal, PieceType::PAWN>() const {
  auto pushMoves = [&]<Color color> {
    BitUtils::bitsForEach<>(
        clearRankFromBitboard<promotionRank<color>>(singlePawnPushTargets(color)), [&](int dest) {
          pushIfSafe(MoveFactory::createMove<MoveType::Normal, PieceType::PAWN>(
              dest - Directions::sfamt(forward<color>), dest, *this));
        });
    BitUtils::bitsForEach<>(filterEnemySquares(clearRankFromBitboard<promotionRank<color>>(
        squaresAttackedByPawns(color)), color), [&](int dest) { pushPawnAttackMoves<color>(dest); });
  };
  isWhiteToMove() ? pushMoves.operator()<Color::WHITE>() : pushMoves.operator()<Color::BLACK>();
}

template<>
inline void Position::pushLegalMoves<MoveType::Promotion, PieceType::PAWN>() const {
  auto pushMoves = [&]<Color color> {
    BitUtils::bitsForEach<>(
        filterRankFromBitboard<promotionRank<color>>(singlePawnPushTargets(color)), [&](int dest) {
          pushIfSafe(MoveFactory::createMove<MoveType::Promotion, PieceType::PAWN>(
              dest - Directions::sfamt(forward<color>), dest, *this));
        });
    BitUtils::bitsForEach<>(filterEnemySquares(filterRankFromBitboard<promotionRank<color>>(
        squaresAttackedByPawns(color)), color), [&](int dest) { pushPawnAttackMoves<color>(dest); });
  };
  isWhiteToMove() ? pushMoves.operator()<Color::WHITE>() : pushMoves.operator()<Color::BLACK>();
}

template<>
inline void Position::pushLegalMoves<MoveType::Enpassant, PieceType::PAWN>() const {
  auto pushMoves = [&]<Color color> {
    BitUtils::bitsForEach<>((1ULL << maybeEnpassantSquare().value_or(0ULL)) & ~1ULL, [&](int dest) {
      pushPawnAttackMoves<color>(dest);
    });
  };
  isWhiteToMove() ? pushMoves.operator()<Color::WHITE>() : pushMoves.operator()<Color::BLACK>();
}

template<>
inline void Position::pushLegalMoves<MoveType::DoublePawnPush, PieceType::PAWN>() const {
  auto pushMoves = [&]<Color color> {
    BitUtils::bitsForEach<>(
        Position::stepBitboard<forward<color>>(singlePawnPushTargets(color), [](u64 bb) {
          return Position::filterRankFromBitboard<pawnRank<color> + rankDelta<color>>(bb);
        }), [&](int dest) {
          pushIfSafe(MoveFactory::createMove<MoveType::DoublePawnPush, PieceType::PAWN>(
              dest - 2 * Directions::sfamt(forward<color>), dest, *this));
        });
  };
  isWhiteToMove() ? pushMoves.operator()<Color::WHITE>() : pushMoves.operator()<Color::BLACK>();
}

template<>
inline void Position::pushLegalMoves<MoveType::Castle, PieceType::KING>() const {
  auto castlingDests = availableCastlingDests(sideToMove());
  u64 castlingTargets = std::accumulate(castlingDests.begin(), castlingDests.end(),
      0ULL, [](u64 dests, int sq) { return dests | (1ULL << sq); });
  auto pushMoves = [&]<Color color> {
    BitUtils::bitsForEach<>(castlingTargets, [&](int dest) {
        pushIfSafe(MoveFactory::createMove<MoveType::Castle, PieceType::KING>(
            kingSquare(sideToMove()), dest, *this));
        });
  };
  isWhiteToMove() ? pushMoves.operator()<Color::WHITE>() : pushMoves.operator()<Color::BLACK>();
}
