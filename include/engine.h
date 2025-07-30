#pragma once

#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <type_traits>
#include <variant>

#include "board_state_snapshot.h"
#include "move_list.h"
#include "pieces.h"
#include "position.h"
#include "search.h"
#include "state_stack.h"
#include "transposition_table.h"
#include "zobrist_hasher.h"

class SearchEngine {
private:
  Position position_;
  StateStack<BoardStateSnapshot> undoStack_;
  TranspositionTable tt_;
  u64 nodesSearched_;

  // private constructor for testing
  explicit SearchEngine(const Position& position, size_t ttSize) : position_{position}, undoStack_{}, tt_{ttSize}, nodesSearched_{0ULL} {};

  struct Line {
    std::optional<MoveVariant> move = std::nullopt;
    int score;
  };

  struct TTEvaluation {
    int score;
    Bound bound = Bound::EXACT;
    TTEvaluation(int score, Bound bound) : score(score), bound(bound) {};
  };

  [[nodiscard]] static std::pair<int, int> getStep(MoveVariant moveVariant) {
    return std::visit(
      [](auto&& move) -> std::pair<int, int> { return { move.start(), move.end() }; },
      moveVariant
    );
  }

  [[nodiscard]] static Bound getBound(int score, int alpha, int beta) {
    if (score < alpha) return Bound::UPPER;
    if (score > beta) return Bound::LOWER;
    return Bound::EXACT;
  };

  [[nodiscard]] std::optional<TTEvaluation> ttEvalLookup(int depth) {
    if (auto maybeEntry = tt_.probe(position_.getHash()); maybeEntry)
      if (auto& ttEntry = *(*maybeEntry); ttEntry.hasAtLeastDepth(depth))
        return TTEvaluation( ttEntry.getEval(), ttEntry.getBound() );
    return std::nullopt;
  }

  void ttEvalStore(int score, int depth, int alpha, int beta, MoveVariant bestMove) {
    tt_.store(position_.getHash(), score, depth, getBound(score, alpha, beta), getStep(bestMove));
  }

  int getTTScoreOrSearch(int& alpha, int& beta, int depth, bool isMaximizingPlayer, Color color) {
    if (auto ttEval = ttEvalLookup(depth); ttEval.has_value()) {
      switch (ttEval->bound) {
        case Bound::EXACT:
          return ttEval->score;
        case Bound::UPPER:
          beta = std::min(beta, ttEval->score); break;
        case Bound::LOWER:
          alpha = std::max(alpha, ttEval->score); break;
      }
    }
    return alphaBeta(alpha, beta, depth-1, !isMaximizingPlayer, opposite(color)).score;
  }

  Line alphaBeta(int alpha, int beta, int depth, bool isMaximizingPlayer, Color color) {
    if (!depth) // end of search reached, return final eval
      return { .score = position_.evaluation() };
    const auto possibleMoves = position_.legalMoves();
    nodesSearched_ += possibleMoves.size();
    if (possibleMoves.isEmpty()) // end of game reached, return final eval
      return { .score = position_.evaluation() };
    int originalAlpha = alpha, originalBeta = beta;
    auto bestMove = *(possibleMoves.begin());
    for (const auto move : possibleMoves) {
      advance(move);
      int eval = getTTScoreOrSearch(alpha, beta, depth, isMaximizingPlayer, color);
      backtrack(move);
      if (isMaximizingPlayer ? (eval > alpha) : (eval < beta))
        bestMove = move, isMaximizingPlayer ? (alpha = eval) : (beta = eval);
      if (beta <= alpha)
        break; // prune branch
    }
    ttEvalStore(isMaximizingPlayer ? alpha : beta, depth, originalAlpha, originalBeta, bestMove);
    return Line{ .move = bestMove, .score = isMaximizingPlayer ? alpha : beta };
  }

public:
  SearchEngine() : position_{}, undoStack_{}, tt_{}, nodesSearched_{0ULL} {};
  explicit SearchEngine(const std::string& fen) : position_{fen}, undoStack_{}, tt_{}, nodesSearched_{0ULL} {};

  // Factory method for testing - creates engine without transposition table allocation
  [[nodiscard]] static SearchEngine withEmptyTTForTesting(const Position& position) {
    return SearchEngine(position, 0);
  }

  // prevent copying (very memory intensive)
  SearchEngine(const SearchEngine&) = delete; 
  SearchEngine operator=(const SearchEngine&) = delete;

  void advance(MoveVariant variant) {
    undoStack_.push(position_.getStateSnapshot());
    std::visit(
        [this](auto&& move) {
          position_.applyMove<std::decay_t<decltype(move)>::type>( move );
        }, variant);
  }

  void backtrack(MoveVariant variant) {
    std::visit(
        [this](auto&& move) {
          position_.undoMove<std::decay_t<decltype(move)>::type>( move, undoStack_.pop() );
        }, variant);
  }

  MoveVariant search(int depth = SEARCH_DEPTH) {
    undoStack_.clear();
    auto [ maybeMove, score ] = alphaBeta(
        NEGINF, POSINF, depth, position_.isWhiteToMove(), position_.sideToMove());
    assert(maybeMove && "game is over, no further moves can be played!");
    assert(position_.legalMoves().contains(*maybeMove) && "move not found in legal moves");
    advance(*maybeMove);
    return *maybeMove;
  }

  const Position& getPosition() const { return position_; }

  void dumpPosition() const {
    std::cout << position_ << '\n';
  }  

  u64 getNodesSearchedCount() const { return nodesSearched_; }

};
