#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <limits>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "move/move.h"
#include "util/platform.h"

enum class Bound
{
    EXACT,
    LOWER,
    UPPER
};

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

template <Field field>
struct FieldTraits
{
    static constexpr size_t index = -1;
    static constexpr int width = -1;
    static constexpr int offset = -1;
};

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
    // start, end, and the promotion piece that distinguishes the four
    // promotions sharing a start/end pair (Move::ORDERING_KEY_WIDTH)
    static constexpr int width = 14;
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
    // Holds up to MAX_SEARCH_DEPTH; the previous 4 bits silently wrapped once
    // iterative deepening passed depth 15.
    static constexpr int width = 7;
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

template <size_t... Is>
constexpr bool verifyTraits(std::index_sequence<Is...>) noexcept
{
    return (... && (FieldTraits<IndexToField<Is>::field>::index == Is));
}

static_assert(verifyTraits(std::make_index_sequence<FIELD_COUNT>{}), "Field traits index mismatch");

// The fields tile the 12-byte entry exactly, which also lands SCORE and KEY on
// byte boundaries.
constexpr int PACKED_ENTRY_BYTES = 12;
static_assert(FieldTraits<Field::KEY>::offset + FieldTraits<Field::KEY>::width ==
                  PACKED_ENTRY_BYTES * 8,
              "Packed TT entry must fill exactly 12 bytes with no spare bits");
static_assert(FieldTraits<Field::BESTMOVE>::width == Move::ORDERING_KEY_WIDTH,
              "TT best-move field must hold a full Move::orderingKey()");

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

// Entry i has the low i bits set, so indexing by a field width masks exactly
// that field. Index 0 is empty and index N is the full width.
template <std::integral T, int N, size_t... Is>
    requires std::is_unsigned_v<T>
static constexpr std::array<T, N + 1> generateLowerMasks(std::index_sequence<Is...>) noexcept
{
    constexpr T fullMask = std::numeric_limits<T>::max();
    return {static_cast<T>(Is == 0 ? T{0} : fullMask >> (N - Is))...};
}

template <std::integral T, int N = (sizeof(T) * 8), int M = N>
static constexpr std::array<std::array<T, N>, N> subMasks() noexcept
{
    std::array<std::array<T, N>, N> subMasks{};
    T mask = std::numeric_limits<T>::max();
    for (int i = 0; i < N; ++i)
    {
        for (int j = i; j < std::min(i + M, N); ++j)
        {
            subMasks[i][j] =
                (i == j) ? static_cast<T>(1) << i : (mask << i) & (mask >> (N - j - 1));
        }
    }
    return subMasks;
}

template <std::integral T, int N = (sizeof(T) * 8)>
static constexpr std::array<T, N + 1> lowMasks() noexcept
{
    return generateLowerMasks<T, N>(std::make_index_sequence<N + 1>{});
}

// Bitfield utilities
constexpr int BYTE_SIZE = 8;
constexpr auto U64_MASKS = lowMasks<u64>();
constexpr auto U64_SUBMASKS = subMasks<u64, sizeof(u64) * BYTE_SIZE, BYTE_SIZE>();

static_assert(U64_MASKS[0] == 0ULL, "U64_MASKS[i] must set the low i bits");
static_assert(U64_MASKS[56] == 0x00ffffffffffffffULL, "U64_MASKS[i] must set the low i bits");
static_assert(U64_MASKS[64] == ~0ULL, "U64_MASKS[i] must set the low i bits");

template <Field field>
constexpr std::pair<int, int> divmod() noexcept
{
    return {detail::FieldTraits<field>::offset / BYTE_SIZE,
            detail::FieldTraits<field>::offset % BYTE_SIZE};
}

struct PackedTTEntry
{
  private:
    template <Field field, std::integral T>
    inline static void writeBits(std::array<u8, 12>& pack, T value) noexcept
    {
        auto [byteOffset, bitOffset] = divmod<field>();
        int bitWidth = detail::FieldTraits<field>::width;
        int bitsToPack = std::min(bitWidth, BYTE_SIZE - bitOffset);

        pack[byteOffset] &=
            ~U64_SUBMASKS[bitOffset][bitOffset + bitsToPack - 1]; // clear bits to write into
        pack[byteOffset] |= (static_cast<u8>(value & U64_SUBMASKS[0][bitsToPack - 1]) << bitOffset);

        ++byteOffset; // move to next byte
        for (int packedBits = bitsToPack; packedBits < bitWidth; ++byteOffset)
        { // writing to the front of next byte
            bitsToPack = std::min(bitWidth - packedBits, BYTE_SIZE);
            pack[byteOffset] &= ~U64_SUBMASKS[0][bitsToPack - 1]; // clear bits to write into
            pack[byteOffset] |=
                ((value & U64_SUBMASKS[packedBits][packedBits + bitsToPack - 1]) >> packedBits);
            packedBits += bitsToPack;
        }
    }

    template <Field field, std::integral T>
    inline static T readBits(const std::array<u8, 12>& pack) noexcept
    {
        auto [byteOffset, bitOffset] = divmod<field>();
        int bitWidth = detail::FieldTraits<field>::width;

        int bitsToUnpack = std::min(bitWidth, BYTE_SIZE - bitOffset);

        T unpacked = 0;
        unpacked |=
            (pack[byteOffset] & U64_SUBMASKS[bitOffset][bitOffset + bitsToUnpack - 1]) >> bitOffset;

        ++byteOffset;
        for (int unpackedBits = bitsToUnpack; unpackedBits < bitWidth; ++byteOffset)
        {
            bitsToUnpack = std::min(bitWidth - unpackedBits, BYTE_SIZE);
            unpacked |= ((pack[byteOffset] & U64_SUBMASKS[0][bitsToUnpack - 1]) << unpackedBits);
            unpackedBits += bitsToUnpack;
        }

        return unpacked;
    }

    template <typename... Ts, size_t... Is>
    inline static void insertBits(std::array<u8, 12>& pack, const std::tuple<Ts...>& tp,
                                  std::index_sequence<Is...>) noexcept
    {
        (writeBits<detail::IndexToField<Is>::field, Ts>(pack, std::get<Is>(tp)), ...);
    }

    std::array<u8, 12> pack_;

    template <Field field, typename T = int>
    [[nodiscard]] inline T getField() const noexcept
    {
        return readBits<field, T>(pack_);
    }

    template <Field field, typename T>
        requires std::is_integral_v<T> || std::is_enum_v<T>
    inline void setField(T value) noexcept
    {
        if constexpr (std::is_enum_v<T>)
        {
            using underlying = std::underlying_type_t<T>;
            writeBits<field, underlying>(pack_, static_cast<underlying>(value));
        }
        else
        {
            writeBits<field, T>(pack_, value);
        }
    }

  public:
    PackedTTEntry() noexcept : pack_{} { clear(); }

    PackedTTEntry(u64 key, int score, int depth, Bound bound, u16 bestMove) noexcept : pack_{}
    {
        u32 vacantFlag = 0;
        u32 encodedMove = bestMove;
        u32 boundValue = static_cast<int>(bound);
        u32 depthValue = depth;
        i16 scoreValue = static_cast<i16>(score);
        u64 truncatedKey = key & U64_MASKS[detail::FieldTraits<Field::KEY>::width];

        auto options = std::make_tuple(vacantFlag, encodedMove, boundValue, depthValue, scoreValue,
                                       truncatedKey);
        insertBits(pack_, options, std::make_index_sequence<FIELD_COUNT>{});
    }

    // observer methods
    [[nodiscard]] inline bool isOccupied() const noexcept { return !getField<Field::VACANT>(); }
    [[nodiscard]] inline bool hasMatchingKey(u64 otherKey) const noexcept
    {
        return getField<Field::KEY, u64>() ==
               (otherKey & U64_MASKS[detail::FieldTraits<Field::KEY>::width]);
    }
    [[nodiscard]] inline Bound getBound() const noexcept { return Bound(getField<Field::BOUND>()); }
    [[nodiscard]] inline int getEval() const noexcept { return getField<Field::SCORE, i16>(); }
    [[nodiscard]] inline bool hasAtLeastDepth(int depth) const noexcept
    {
        return getField<Field::DEPTH>() >= depth;
    }
    // Move::orderingKey() of the stored best move.
    [[nodiscard]] inline u16 getMove() const noexcept
    {
        return getField<Field::BESTMOVE, u16>();
    }
    [[nodiscard]] inline int getDepth() const noexcept { return getField<Field::DEPTH>(); }

    // mutators
    inline void setEval(int score) noexcept { setField<Field::SCORE>(score); }
    inline void setBound(Bound bound) noexcept { setField<Field::BOUND>(bound); }

    template <typename... FieldDataTypes>
    inline void update(FieldDataTypes... fds) noexcept
    {
        (setField<decltype(fds)::field_type, decltype(fds)::value_type>(fds.get()), ...);
    }

    inline void clear() noexcept { setField<Field::VACANT>(true); }
};

class TranspositionTable
{
  private:
    static constexpr size_t DEFAULT_HASH_SIZE = (1UL << 20) + 7;

    std::vector<PackedTTEntry> table_;
    size_t size_;

  public:
    explicit TranspositionTable(size_t size = DEFAULT_HASH_SIZE) noexcept
        : table_{size}, size_{size}
    {
    }

    void store(u64 key, int score, int depth, Bound bound, u16 bestMove) noexcept
    {
        size_t index = key % size_;
        table_[index] = PackedTTEntry{key, score, depth, bound, bestMove};
    }

    [[nodiscard]] std::optional<const PackedTTEntry*> probe(u64 key) const noexcept
    {
        size_t index = key % size_;
        auto& entry = table_[index];
        return (entry.isOccupied() && entry.hasMatchingKey(key))
                   ? std::optional<const PackedTTEntry*>(&entry)
                   : std::nullopt;
    }

    void clear() noexcept
    {
        for (auto& entry : table_)
            entry.clear();
    }
};
