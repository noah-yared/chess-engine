#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <random>
#include <type_traits>

#include "board/constants.h"
#include "util/platform.h"

template <typename RandomEngine>
concept HasResultType = requires { typename RandomEngine::result_type; };

template <typename RandomEngine, typename SeedType>
concept Seedable = requires(RandomEngine rng, SeedType rngSeed) { rng.seed(rngSeed); };

template <typename Container>
concept Iterable = requires(Container c) { c.begin(); };

template <typename RandomEngine,
          typename RandomType = std::conditional_t<HasResultType<RandomEngine>,
                                                   typename RandomEngine::result_type, void>>
    requires std::uniform_random_bit_generator<RandomEngine> &&
             std::convertible_to<RandomType, u64> && Seedable<RandomEngine, RandomType>
class Zobrist
{
  public:
    std::array<std::array<u64, SQUARES>, NUM_COLORS * NUM_PIECE_TYPES> pieceKeys;
    // const std::array<std::array<u64, NUM_CASTLING_PRIVILEGE_STATES>, NUM_CASTLING_RIGHTS>
    // castlingKeys;
    std::array<u64, CASTLING_COMBINATIONS> castlingKeys;
    std::array<u64, FILES> enpassantKeys;
    u64 turnKey;

    explicit Zobrist(u64 seed = DEFAULT_SEED) noexcept : rng_{seed}, seed_{seed}
    {
        auto keys = std::tie(pieceKeys, castlingKeys, enpassantKeys, turnKey);
        fillAll(rng_, keys);
    };

  private:
    // static constexpr u64 DEFAULT_SEED = 0XC0FFEECAFEULL;

    RandomEngine rng_;
    RandomType seed_;

    template <typename Container>
        requires std::is_integral_v<Container> || Iterable<Container>
    static void fillWithRandom(Container& c, RandomEngine& rng_) noexcept
    {
        std::uniform_int_distribution<u64> rdist;
        if constexpr (Iterable<Container>)
        {
            for (auto& e : c)
                fillWithRandom(e, rng_);
        }
        else
        {
            c = rdist(rng_);
        }
    }

    template <typename KeyTupleType>
    static void fillAll(RandomEngine& rng_, KeyTupleType& keyTuple) noexcept
    {
        std::apply([=](auto&... keys) mutable noexcept -> void
                   { (fillWithRandom(keys, rng_), ...); }, keyTuple);
    }
};
