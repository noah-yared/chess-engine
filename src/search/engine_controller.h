#pragma once

#include <array>
#include <type_traits>
#include <variant>

#include "board/position.h"
#include "move/move.h"
#include "search/search_types.h"
#include "search/searcher.h"
#include "search/strength.h"
#include "search/transposition_table.h"

class EngineController
{
  public:
    EngineController() noexcept : position_{}, tt_{} {};
    explicit EngineController(const Position& position) : position_{position}, tt_{} {};
    explicit EngineController(const std::string& fen) : position_{fen}, tt_{} {};

    SearchResult search(const SearchConfig& config)
    {
        return Searcher::search(position_, config, config.options.useTT ? &tt_ : nullptr);
    }

    // useful for quick tests
    SearchResult search(int depth = DEFAULT_SEARCH_DEPTH)
    {
        return search(SearchConfig::fixedDepth(depth));
    }

    MoveVariant playEngineMove(const SearchConfig& config)
    {
        auto result = search(config);
        advance(result.bestMove);
        return result.bestMove;
    }

    MoveVariant playEngineMove(Strength strength)
    {
        return playEngineMove(
            SearchConfig::fixedTime(computeTimeBudgetMS(strength), MAX_SEARCH_DEPTH));
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

    [[nodiscard]] static int computeTimeBudgetMS(Strength strength)
    {
        std::array<int, static_cast<int>(Strength::NUM_LEVELS)> timeBudgetsMS = {500,  1000, 2000,
                                                                                 4000, 8000, 10000};
        return timeBudgetsMS[static_cast<int>(strength)];
    }
};
