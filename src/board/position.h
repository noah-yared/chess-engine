#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <optional>
#include <random>
#include <type_traits>
#include <variant>

#include "board/bitboards.h"
#include "board/board_state.h"
#include "board/constants.h"
#include "board/directions.h"
#include "board/position_update.h"
#include "board/precomputed_attacks.h"
#include "board/zobrist_hasher.h"
#include "eval/evaluate.h"
#include "move/move.h"
#include "move/move_list.h"
#include "move/piece_square_deltas.h"
#include "util/bit_utils.h"
#include "util/static_vector.h"

class Position
{
  public:
    using RNG = std::mt19937_64; // random number generator for hashing function

    friend std::ostream& operator<<(std::ostream& os, const Position& pos) noexcept;
    friend void debug(Position& pos);

    /////////////////////////
    // Constructors        //
    /////////////////////////
    Position() noexcept
        : bitboards_{}, state_{}, hash_{ZobristHasher<RNG>::initialZobristHash()} {};
    explicit Position(const std::string& fen) noexcept
        : bitboards_{fen, FromFEN{}}, state_{fen},
          hash_{ZobristHasher<RNG>::computeZobristHash(bitboards_, state_)} {};

    /////////////////////////
    // Factory Methods     //
    /////////////////////////
    static Position fromStartingPosition() noexcept { return Position(std::string(STARTING_FEN)); }

    static Position fromAscii(const std::string& asciiBoard, const std::string turn = "w",
                              const std::string castlingRights = "-",
                              const std::string enpassant = "-") noexcept
    {
        return {Bitboards(asciiBoard, FromAsciiBoard{}),
                BoardState(turn, castlingRights, enpassant)};
    }

    /////////////////////////
    // Position Loading    //
    /////////////////////////
    void loadFEN(const std::string& fen) noexcept
    {
        bitboards_ = Bitboards(fen, FromFEN{});
        state_ = BoardState(fen);
        hash_ = ZobristHasher<RNG>::computeZobristHash(bitboards_, state_);
    }

    void loadAsciiBoard(const std::string& asciiBoard, const std::string turn = "w",
                        const std::string castlingRights = "-",
                        const std::string enpassant = "-") noexcept
    {
        bitboards_ = Bitboards(asciiBoard, FromAsciiBoard{});
        state_ = BoardState(turn, castlingRights, enpassant);
        hash_ = ZobristHasher<RNG>::computeZobristHash(bitboards_, state_);
    }

    /////////////////////////
    // Move Operations     //
    /////////////////////////
    // Callers that already know the move type at compile time (move generation's
    // legality check, the search's own dispatch) should name it explicitly and
    // skip the switch below.
    template <MoveType mType>
    void applyMove(const Move move) noexcept
    {
        const auto deltas = PieceSquareDeltas<mType>::generate(move);
        updateBitboards<mType>(move, deltas);
        auto extractedPrevState = state_.extract();
        updateState<mType>(move);
        updateHash<mType>(move, deltas, BoardState(extractedPrevState), state_);
    }

    template <MoveType mType>
    void undoMove(const Move move, const BoardStateSnapshot previousSnapshot) noexcept
    {
        revertBitboards<mType>(move);
        auto [prevState, prevHash] = previousSnapshot;
        state_.revert(prevState), hash_ = prevHash;
    }

    void applyMove(const Move move) noexcept
    {
        switch (move.type())
        {
        case MoveType::Normal:
            return applyMove<MoveType::Normal>(move);
        case MoveType::Enpassant:
            return applyMove<MoveType::Enpassant>(move);
        case MoveType::Promotion:
            return applyMove<MoveType::Promotion>(move);
        case MoveType::Castle:
            return applyMove<MoveType::Castle>(move);
        case MoveType::DoublePawnPush:
            return applyMove<MoveType::DoublePawnPush>(move);
        }
    }

    void undoMove(const Move move, const BoardStateSnapshot previousSnapshot) noexcept
    {
        switch (move.type())
        {
        case MoveType::Normal:
            return undoMove<MoveType::Normal>(move, previousSnapshot);
        case MoveType::Enpassant:
            return undoMove<MoveType::Enpassant>(move, previousSnapshot);
        case MoveType::Promotion:
            return undoMove<MoveType::Promotion>(move, previousSnapshot);
        case MoveType::Castle:
            return undoMove<MoveType::Castle>(move, previousSnapshot);
        case MoveType::DoublePawnPush:
            return undoMove<MoveType::DoublePawnPush>(move, previousSnapshot);
        }
    }

    /////////////////////////
    // Static Evaluation   //
    /////////////////////////
    template <bool easyMode = false>
    [[nodiscard]] int evaluation() const noexcept
    {
        if constexpr (easyMode)
        {
            const int randomNoise = (rand() % 200) - 100;
            return Evaluator::evaluate(bitboards_) + randomNoise;
        }
        else
        {
            return Evaluator::evaluate(bitboards_);
        }
    }
    // for benchmarking/debugging
    [[nodiscard]] int evaluation_v1() const noexcept { return Evaluator::evaluate_v1(bitboards_); }
    [[nodiscard]] int evaluation_v2() const noexcept { return Evaluator::evaluate_v2(bitboards_); }

    /////////////////////////
    // State Interface     //
    /////////////////////////
    [[nodiscard]] BoardStateSnapshot getStateSnapshot() const noexcept
    {
        return {.state = state_.extract(), .hash = hash_};
    }
    [[nodiscard]] u64 getHash() const noexcept { return hash_; }
    [[nodiscard]] std::optional<int> maybeEnpassantSquare() const noexcept
    {
        return state_.getEnpassantSquare();
    }
    [[nodiscard]] int castlingRights() const noexcept { return state_.castlingBits(); }
    template <Color color>
    [[nodiscard]] StaticVector<int, 2> availableCastlingDests() const noexcept
    {
        return state_.availableCastlingDestinations<color>();
    }
    [[nodiscard]] StaticVector<int, 2> availableCastlingDests(Color color) const noexcept
    {
        return state_.availableCastlingDestinations(color);
    }
    [[nodiscard]] Color sideToMove() const noexcept { return state_.getTurn(); }
    [[nodiscard]] bool isWhiteToMove() const noexcept { return state_.getTurn() == Color::WHITE; }

    /////////////////////////
    // Bitboards Interface //
    /////////////////////////
    // King square
    [[nodiscard]] int kingSquare(Color color) const noexcept { return bitboards_.king(color); }

    // Piece occupancy queries
    [[nodiscard]] u64 getPieceBitboard(PieceType pType, Color color) const noexcept
    {
        return bitboards_.bb(pType, color);
    }
    [[nodiscard]] PieceType getPieceOccupyingSquare(int square) const noexcept
    {
        return bitboards_.getPieceType(square);
    }
    [[nodiscard]] PieceType getPieceOccupyingSquare(int square, Color color) const noexcept
    {
        return bitboards_.getPieceType(square, color);
    }
    [[nodiscard]] bool isPieceOccupyingSquare(PieceType pType, Color color,
                                              int square) const noexcept
    {
        return bitboards_.bb(pType, color) & 1ULL << square;
    }
    [[nodiscard]] bool isAllyOccupyingSquare(int square, Color color) const noexcept
    {
        return bitboards_.allyBB(color) & 1ULL << square;
    }
    [[nodiscard]] bool isEnemyOccupyingSquare(int square, Color color) const noexcept
    {
        return bitboards_.opposingBB(color) & 1ULL << square;
    }
    [[nodiscard]] bool isSquareOccupied(int square) const noexcept
    {
        return bitboards_.combinedBB() & 1ULL << square;
    }

    // Bitboard filters
    [[nodiscard]] u64 filterOccupiedSquares(u64 bb) const noexcept
    {
        return bb & bitboards_.combinedBB();
    }
    [[nodiscard]] u64 filterAllySquares(u64 bb, Color color) const noexcept
    {
        return bb & bitboards_.allyBB(color);
    }
    [[nodiscard]] u64 filterEnemySquares(u64 bb, Color color) const noexcept
    {
        return bb & bitboards_.opposingBB(color);
    }

    // Bitboard clearers
    [[nodiscard]] u64 clearOccupiedSquares(u64 bb) const noexcept
    {
        return bb & ~bitboards_.combinedBB();
    }
    [[nodiscard]] u64 clearAllySquares(u64 bb, Color color) const noexcept
    {
        return bb & ~bitboards_.allyBB(color);
    }
    [[nodiscard]] u64 clearEnemySquares(u64 bb, Color color) const noexcept
    {
        return bb & ~bitboards_.opposingBB(color);
    }

    /////////////////////////
    // Utility Methods     //
    /////////////////////////
    std::string stringifyBitboards() const noexcept { return bitboards_.toString(); }
    std::string stringifyBoardState() const noexcept { return state_.toString(); }
    bool areBitboardsConsistent() const noexcept { return bitboards_.isConsistent(); }
    std::string toFen() const noexcept
    {
        std::stringstream ss;
        ss << bitboards_.parsePiecePlacement() << " " << state_.parseTurn() << " "
           << state_.parseCastlingRights() << " " << state_.parseEnpassantSquare()
           << " 0 1"; // not tracking halfmove clock or fullmove number
        return ss.str();
    }

    /////////////////////////
    // Operators           //
    /////////////////////////
    bool operator==(const Position& other) const noexcept
    {
        return bitboards_ == other.bitboards_ && state_ == other.state_ && hash_ == other.hash_;
    }
    bool operator!=(const Position& other) const noexcept { return !(operator==(other)); }

  private:
    Bitboards bitboards_;
    BoardState state_;
    u64 hash_;

    /////////////////////////
    // Private Constructor //
    /////////////////////////
    Position(Bitboards bitboards, BoardState state) noexcept
        : bitboards_{bitboards}, state_{state},
          hash_{ZobristHasher<RNG>::computeZobristHash(bitboards, state)} {};

    /////////////////////////
    // State Updates       //
    /////////////////////////
    template <MoveType mType, typename Container>
    void updateBitboards(const Move move, const Container& deltas) noexcept
    {
        for (const auto [key, square] : deltas)
            bitboards_.togglePieceSquare(key, square);
    }

    template <MoveType mType>
    void revertBitboards(const Move move) noexcept
    {
        for (const auto [key, square] : PieceSquareDeltas<mType>::generate(move))
            bitboards_.togglePieceSquare(key, square);
    }

    template <MoveType mType>
    void updateState(const Move move) noexcept
    {
        updateEnpassantSquare<mType>(move, state_);
        updateCastlingPrivs<mType>(move, state_);
        updateTurn<mType>(move, state_);
    }

    template <MoveType mType, typename Container>
    void updateHash(const Move move, const Container& deltas, const BoardState prevState,
                    const BoardState newState) noexcept
    {
        hash_ ^= ZobristHasher<RNG>::getHashUpdateMask<mType>(
            move, deltas, prevState.getEnpassantSquare(), prevState.castlingBits(),
            newState.castlingBits());
    }
};

inline std::ostream& operator<<(std::ostream& os, const Position& pos) noexcept
{
    os << "Position = {\n"
       << "  state: " << pos.stringifyBoardState() << ",\n"
       << "  hash: " << std::hex << pos.getHash() << std::dec << ",\n"
       << "  board:\n"
       << pos.stringifyBitboards() << "}\n";
    return os;
};
