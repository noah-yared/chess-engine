#pragma once

#include <array>
#include <type_traits>
#include <utility>

#include "platform.h"

enum class Bound
{
    EXACT,
    LOWER,
    UPPER
};

// Bitfield configuration
enum class Field
{
    INVALID = -1,
    VACANT,
    BESTMOVE,
    BOUND,
    DEPTH,
    SCORE,
    KEY,
    COUNT,
};

constexpr int FIELD_COUNT = static_cast<int>(Field::COUNT);

namespace detail
{

// Field Traits
template <Field field>
struct FieldTraits
{
    static constexpr size_t index = -1;
    static constexpr int width = -1;
    static constexpr int offset = -1;
};

// Index Traits
template <size_t index>
struct IndexToField
{
    static constexpr Field field = Field(-1);
};

template <>
struct FieldTraits<Field::VACANT>
{
    static constexpr size_t index = 0;
    static constexpr int width = 1;
    static constexpr int offset = 0;
};

template <>
struct IndexToField<0>
{
    static constexpr Field field = Field::VACANT;
};

template <>
struct FieldTraits<Field::BESTMOVE>
{
    static constexpr size_t index = 1;
    static constexpr int width = 12;
    static constexpr int offset = FieldTraits<IndexToField<index - 1>::field>::offset +
                                  FieldTraits<IndexToField<index - 1>::field>::width;
};

template <>
struct IndexToField<1>
{
    static constexpr Field field = Field::BESTMOVE;
};

template <>
struct FieldTraits<Field::BOUND>
{
    static constexpr size_t index = 2;
    static constexpr int width = 2;
    static constexpr int offset = FieldTraits<IndexToField<index - 1>::field>::offset +
                                  FieldTraits<IndexToField<index - 1>::field>::width;
};

template <>
struct IndexToField<2>
{
    static constexpr Field field = Field::BOUND;
};

template <>
struct FieldTraits<Field::DEPTH>
{
    static constexpr size_t index = 3;
    static constexpr int width = 4;
    static constexpr int offset = FieldTraits<IndexToField<index - 1>::field>::offset +
                                  FieldTraits<IndexToField<index - 1>::field>::width;
};

template <>
struct IndexToField<3>
{
    static constexpr Field field = Field::DEPTH;
};

template <>
struct FieldTraits<Field::SCORE>
{
    static constexpr size_t index = 4;
    static constexpr int width = 16;
    static constexpr int offset = FieldTraits<IndexToField<index - 1>::field>::offset +
                                  FieldTraits<IndexToField<index - 1>::field>::width;
};

template <>
struct IndexToField<4>
{
    static constexpr Field field = Field::SCORE;
};

template <>
struct FieldTraits<Field::KEY>
{
    static constexpr size_t index = 5;
    static constexpr int width = 56;
    static constexpr int offset = FieldTraits<IndexToField<index - 1>::field>::offset +
                                  FieldTraits<IndexToField<index - 1>::field>::width;
};

template <>
struct IndexToField<5>
{
    static constexpr Field field = Field::KEY;
};

// Verify all field traits are correctly defined
template <size_t... Is>
constexpr bool verifyTraits(std::index_sequence<Is...>) noexcept
{
    return (... && (FieldTraits<IndexToField<Is>::field>::index == Is));
}

static_assert(verifyTraits(std::make_index_sequence<FIELD_COUNT>{}), "Field traits index mismatch");

// Field data types to store wrap data for each field
template <Field field, typename T>
struct FieldData
{
    static constexpr Field field_type = field;
    using value_type = T;

    constexpr explicit FieldData(T value) noexcept : value{value} {};

    constexpr T get() const noexcept { return value; }
    constexpr void set(T value) noexcept { this->value = value; }

  private:
    T value;
};

} // namespace detail

// Helper functions
template <std::integral T, int N, size_t... Is>
    requires std::is_unsigned_v<T>
static constexpr std::array<T, N + 1> generateMasks(std::index_sequence<Is...>) noexcept
{
    T fullMask = std::numeric_limits<T>::max();
    return {static_cast<T>(((Is == 0) ? fullMask : (fullMask >> (N - Is))))...};
}

template <std::integral T, int N = (sizeof(T) * 8)>
static constexpr std::array<T, N + 1> masks() noexcept
{
    return generateMasks<T, N>(std::make_index_sequence<N + 1>{});
}
