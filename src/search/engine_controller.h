#pragma once

#include <type_traits>
#include <variant>

#include "board/position.h"
#include "move/move.h"
#include "search/difficulty.h"
#include "search/search_types.h"
#include "search/searcher.h"
#include "search/transposition_table.h"

class EngineController
{
  public:
    EngineController() noexcept : position_{}, tt_{} {};
    explicit EngineController(const Position& position) : position_{position}, tt_{} {};
    explicit EngineController(const std::string& fen) : position_{fen}, tt_{} {};

    SearchResult search(int depth = DEFAULT_SEARCH_DEPTH)
    {
        SearchConfig config(depth);
        return Searcher::search(position_, config, &tt_);
    }

    SearchResult search(Difficulty difficulty, int depth = DEFAULT_SEARCH_DEPTH)
    {
        // TODO: add support for engine difficulty
        (void)difficulty;

        SearchConfig config(depth);
        return Searcher::search(position_, config, &tt_);
    }

    MoveVariant playEngineMove(int depth = DEFAULT_SEARCH_DEPTH)
    {
        auto result = search(depth);
        advance(result.bestMove);
        return result.bestMove;
    }

    MoveVariant playEngineMove(Difficulty difficulty, int depth = DEFAULT_SEARCH_DEPTH)
    {
        auto result = search(difficulty, depth);
        advance(result.bestMove);
        return result.bestMove;
    }

    void advance(MoveVariant move) noexcept
    {
        std::visit([this](auto&& arg) noexcept
                   { position_.applyMove<std::decay_t<decltype(arg)>::type>(arg); }, move);
    }

    void setPosition(const std::string& fen) noexcept { position_ = Position(fen); }
    void setPosition(const Position& position) noexcept { position_ = position; }

    [[nodiscard]] const Position& position() const noexcept { return position_; }
    [[nodiscard]] Color turn() const noexcept { return position_.sideToMove(); }

  private:
    Position position_;
    TranspositionTable tt_;
};
