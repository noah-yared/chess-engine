#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "move.h"
#include "platform.h"
#include "tt_traits.h"

// Bitfield utilities
constexpr int BYTE_SIZE = 8;
constexpr auto U64_MASKS = lowMasks<u64>();
constexpr auto U64_SUBMASKS = subMasks<u64, sizeof(u64) * BYTE_SIZE, BYTE_SIZE>();

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

        pack[byteOffset] &= ~U64_SUBMASKS[bitOffset][bitOffset + bitsToPack - 1]; // clear bits to write into
        pack[byteOffset] |= (static_cast<u8>(value & U64_SUBMASKS[0][bitsToPack - 1]) << bitOffset);

        ++byteOffset; // move to next byte
        for (int packedBits = bitsToPack; packedBits < bitWidth; ++byteOffset)
        { // writing to the front of next byte
            bitsToPack = std::min(bitWidth - packedBits, BYTE_SIZE);
            pack[byteOffset] &= ~U64_SUBMASKS[0][bitsToPack - 1]; // clear bits to write into
            pack[byteOffset] |= ((value & U64_SUBMASKS[packedBits][packedBits + bitsToPack - 1]) >> packedBits);
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
        unpacked |= (pack[byteOffset] & U64_SUBMASKS[bitOffset][bitOffset + bitsToUnpack - 1]) >> bitOffset;

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
    PackedTTEntry() noexcept : pack_{} {}

    PackedTTEntry(u64 key, int score, int depth, Bound bound, std::pair<int, int> bestMove) noexcept
        : pack_{}
    {
        u32 vacantFlag = 1;
        u32 encodedMove = ((bestMove.first & 0x3f) << 6) | (bestMove.second & 0x3f);
        u32 boundValue = static_cast<int>(bound);
        u32 depthValue = depth;
        i16 scoreValue = score;
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
    [[nodiscard]] inline std::pair<int, int> getMove() const noexcept {
        int field = getField<Field::BESTMOVE>();
        return { field >> 6, field & 0x3f };
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

    void store(u64 key, int score, int depth, Bound bound, std::pair<int, int> bestMove) noexcept
    {
        size_t index = key % size_;
        table_[index] = PackedTTEntry{key, score, depth, bound, bestMove};
    }

    std::optional<PackedTTEntry*> probe(u64 key) noexcept
    {
        size_t index = key % size_;
        auto& entry = table_[index];
        return (entry.isOccupied() && entry.hasMatchingKey(key))
                   ? std::optional<PackedTTEntry*>(&entry)
                   : std::nullopt;
    }

    void clear() noexcept
    {
        for (auto& entry : table_)
            entry.clear();
    }
};
