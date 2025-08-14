#pragma once

#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <type_traits>
#include <variant>

#include "board_state_snapshot.h"
#include "move_generator.h"
#include "move_list.h"
#include "pieces.h"
#include "position.h"
#include "search.h"
#include "state_stack.h"
#include "transposition_table.h"
#include "zobrist_hasher.h"

class SearchEngine
{
  private:
    Position position_;
    StateStack<BoardStateSnapshot> undoStack_;
    TranspositionTable tt_;
    u64 nodesSearched_;
    mutable MoveList moveBuffer_;

    // private constructor for testing
    explicit SearchEngine(const Position& position, size_t ttSize) noexcept
        : position_{position}, undoStack_{}, tt_{ttSize}, nodesSearched_{0ULL}, moveBuffer_{} {};

    struct Line
    {
        std::optional<MoveVariant> move = std::nullopt;
        int score;
    };

    struct TTEvaluation
    {
        int score;
        Bound bound = Bound::EXACT;
        TTEvaluation(int score, Bound bound) noexcept : score(score), bound(bound) {};
    };

    [[nodiscard]] static std::pair<int, int> getStep(MoveVariant moveVariant) noexcept
    {
        return std::visit([](auto&& move) noexcept -> std::pair<int, int>
                          { return {move.start(), move.end()}; }, moveVariant);
    }

    [[nodiscard]] static Bound getBound(int score, int alpha, int beta) noexcept
    {
        if (score < alpha)
            return Bound::UPPER;
        if (score > beta)
            return Bound::LOWER;
        return Bound::EXACT;
    };

    [[nodiscard]] std::optional<TTEvaluation> ttEvalLookup(int depth) noexcept
    {
        if (auto maybeEntry = tt_.probe(position_.getHash()); maybeEntry)
            if (auto& ttEntry = *(*maybeEntry); ttEntry.hasAtLeastDepth(depth))
                return TTEvaluation(ttEntry.getEval(), ttEntry.getBound());
        return std::nullopt;
    }

    void ttEvalStore(int score, int depth, int alpha, int beta, MoveVariant bestMove) noexcept
    {
        tt_.store(position_.getHash(), score, depth, getBound(score, alpha, beta),
                  getStep(bestMove));
    }

    template <Color color>
    int getTTScoreOrSearch(int& alpha, int& beta, int depth) noexcept
    {
        if (auto ttEval = ttEvalLookup(depth); ttEval.has_value())
        {
            switch (ttEval->bound)
            {
            case Bound::EXACT:
                return ttEval->score;
            case Bound::UPPER:
                beta = std::min(beta, ttEval->score);
                break;
            case Bound::LOWER:
                alpha = std::max(alpha, ttEval->score);
                break;
            }
        }
        return alphaBeta<opposite<color>()>(alpha, beta, depth - 1).score;
    }

    template <Color color>
    Line alphaBeta(int alpha, int beta, int depth) noexcept
    {
        if (!depth) // end of search reached, return final eval
            return {.score = position_.evaluation()};

        const auto possibleMoves = legalMoves<color>();
        nodesSearched_ += possibleMoves.size();

        if (possibleMoves.isEmpty()) // end of game reached, return final eval
            return {.score = position_.evaluation()};

        int originalAlpha = alpha, originalBeta = beta;
        constexpr bool isMaximizingPlayer = color == Color::WHITE;
        auto bestMove = possibleMoves[0];

        for (const auto move : possibleMoves)
        {
            advance(move);
            int eval = getTTScoreOrSearch<color>(alpha, beta, depth);
            backtrack(move);
            if constexpr (isMaximizingPlayer)
            {
                if (eval > alpha)
                    bestMove = move, alpha = eval;
            }
            else
            {
                if (eval < beta)
                    bestMove = move, beta = eval;
            }
            if (beta <= alpha)
                break; // prune branch
        }

        if constexpr (isMaximizingPlayer)
            ttEvalStore(alpha, depth, originalAlpha, originalBeta, bestMove);
        else
            ttEvalStore(beta, depth, originalAlpha, originalBeta, bestMove);

        return Line{.move = bestMove, .score = isMaximizingPlayer ? alpha : beta};
    }

  public:
    SearchEngine() noexcept : position_{}, undoStack_{}, tt_{}, nodesSearched_{0ULL} {};
    explicit SearchEngine(const std::string& fen) noexcept
        : position_{fen}, undoStack_{}, tt_{}, nodesSearched_{0ULL} {};

    // Factory method for testing - creates engine without transposition table allocation
    [[nodiscard]] static SearchEngine withEmptyTTForTesting(const Position& position) noexcept
    {
        return SearchEngine(position, 0);
    }

    // prevent copying (very memory intensive)
    SearchEngine(const SearchEngine&) = delete;
    SearchEngine operator=(const SearchEngine&) = delete;

    template <Color color>
    const MoveList& legalMoves() const noexcept
    {
        moveBuffer_.clear();
        MoveGenerator::pushLegalMoves<color>(position_, moveBuffer_);
        return moveBuffer_;
    }

    const Position& position() const noexcept { return position_; }

    Color turn() const noexcept { return position_.sideToMove(); }

    void advance(MoveVariant variant) noexcept
    {
        undoStack_.push(position_.getStateSnapshot());
        std::visit([this](auto&& move) noexcept
                   { position_.applyMove<std::decay_t<decltype(move)>::type>(move); }, variant);
    }

    void backtrack(MoveVariant variant) noexcept
    {
        std::visit(
            [this](auto&& move) noexcept
            { position_.undoMove<std::decay_t<decltype(move)>::type>(move, undoStack_.pop()); },
            variant);
    }

    template <Color color>
    MoveVariant search(int depth = SEARCH_DEPTH) noexcept
    {
        undoStack_.clear();
        moveBuffer_.clear();

        auto [maybeMove, score] = alphaBeta<color>(NEGINF, POSINF, depth);

        assert(maybeMove && "game is over, no further moves can be played!");
        assert(legalMoves<color>().contains(*maybeMove) && "move not found in legal moves");

        advance(*maybeMove);
        return *maybeMove;
    }

    MoveVariant search(int depth = SEARCH_DEPTH) noexcept
    {
        return turn() == Color::WHITE ? search<Color::WHITE>(depth) : search<Color::BLACK>(depth);
    }

    void dumpPosition(std::ostream& os = std::cout) const noexcept { os << position_ << '\n'; }

    u64 nodesSearched() const noexcept { return nodesSearched_; }
};
