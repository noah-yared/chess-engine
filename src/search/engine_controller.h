#pragma once

#include <array>
#include <cassert>
#include <cstdlib>
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
        return playEngineMove(buildStrengthConfig(strength));
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
        std::array<int, static_cast<int>(Strength::NUM_LEVELS)> timeBudgetsMS = {100, 1500, 8000};
        return timeBudgetsMS[static_cast<int>(strength)];
    }

    [[nodiscard]] static SearchConfig buildStrengthConfig(Strength strength)
    {
        switch (strength)
        {
        case Strength::LOW:
            return SearchConfig::fixedTime(computeTimeBudgetMS(strength))
                .withoutQuiescence() // misses tactics/exchanges
                .withoutTT();        // slow down search
        case Strength::MEDIUM:
        case Strength::HIGH:
            return SearchConfig::fixedTime(computeTimeBudgetMS(strength));
        default:
            // should not reach this case
            assert(false && "Invalid strength passed in!");
            std::abort();
        }
    }
};
