#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <format>
#include <limits>
#include <optional>
#include <utility>
#include <vector>
#include <type_traits>

#include "move.h"
#include "platform.h"

enum class Bound { EXACT, LOWER, UPPER };

// Bitfield configuration
enum class Field {
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

namespace detail {

// Field Traits
template<Field field>
struct FieldTraits {
  static constexpr size_t index = -1;
  static constexpr int width = -1;
  static constexpr int offset = -1;
};

// Index Traits
template<size_t index>
struct IndexToField {
  static constexpr Field field = Field(-1);
};

template<>
struct FieldTraits<Field::VACANT> {
  static constexpr size_t index = 0;
  static constexpr int width = 1;
  static constexpr int offset = 0;
};

template<>
struct IndexToField<0> {
  static constexpr Field field = Field::VACANT;
};

template<>
struct FieldTraits<Field::BESTMOVE> {
  static constexpr size_t index = 1;
  static constexpr int width = 12;
  static constexpr int offset = FieldTraits<IndexToField<index-1>::field>::offset + FieldTraits<IndexToField<index-1>::field>::width;
};

template<>
struct IndexToField<1> {
  static constexpr Field field = Field::BESTMOVE;
};

template<>
struct FieldTraits<Field::BOUND> {
  static constexpr size_t index = 2;
  static constexpr int width = 2;
  static constexpr int offset = FieldTraits<IndexToField<index-1>::field>::offset + FieldTraits<IndexToField<index-1>::field>::width;
};

template<>
struct IndexToField<2> {
  static constexpr Field field = Field::BOUND;
};

template<>
struct FieldTraits<Field::DEPTH> {
  static constexpr size_t index = 3;
  static constexpr int width = 4;
  static constexpr int offset = FieldTraits<IndexToField<index-1>::field>::offset + FieldTraits<IndexToField<index-1>::field>::width;
};

template<>
struct IndexToField<3> {
  static constexpr Field field = Field::DEPTH;
};

template<>
struct FieldTraits<Field::SCORE> {
  static constexpr size_t index = 4;
  static constexpr int width = 16;
  static constexpr int offset = FieldTraits<IndexToField<index-1>::field>::offset + FieldTraits<IndexToField<index-1>::field>::width;
};

template<>
struct IndexToField<4> {
  static constexpr Field field = Field::SCORE;
};

template<>
struct FieldTraits<Field::KEY> {
  static constexpr size_t index = 5;
  static constexpr int width = 56;
  static constexpr int offset = FieldTraits<IndexToField<index-1>::field>::offset + FieldTraits<IndexToField<index-1>::field>::width;
};

template<>
struct IndexToField<5> {
  static constexpr Field field = Field::KEY;
};


// Verify all field traits are correctly defined
template<size_t... Is>
constexpr bool verifyTraits(std::index_sequence<Is...>) {
  return (... && (FieldTraits<IndexToField<Is>::field>::index == Is));
}

static_assert(verifyTraits(std::make_index_sequence<FIELD_COUNT>{}), "Field traits index mismatch");

// Field data types to store wrap data for each field
template<Field field, typename T>
struct FieldData {
  static constexpr Field field_type = field;
  using value_type = T;

  constexpr explicit FieldData(T value) : value{value} {};

  constexpr T get() const { return value; }
  constexpr void set(T value) { this->value = value; }

  private:
    T value;
};

} // namespace detail


// Helper functions
template<std::integral T, int N, size_t ...Is>
static constexpr std::array<T, N+1> generateMasks(std::index_sequence<Is...>) {
  T fullMask = std::numeric_limits<T>::max();
  return { static_cast<T>(((Is == 0) ? fullMask : (fullMask >> (N - Is))))... };
}

template<std::integral T, int N = (sizeof(T) * 8)>
static constexpr std::array<T, N+1> masks() { return generateMasks<T, N>(std::make_index_sequence<N+1>{}); }


// Bitfield utilities
constexpr int BYTE_SIZE = 8;
constexpr auto U64_MASKS = masks<u64>();
constexpr auto U8_MASKS = masks<u8>();
template<Field field>
constexpr std::pair<int, int> divmod() {
  return { detail::FieldTraits<field>::offset / BYTE_SIZE,
           detail::FieldTraits<field>::offset % BYTE_SIZE };
}

struct PackedTTEntry {
private:
  template<Field field, std::integral T>
  inline static void writeBits(std::array<u8, 12> &pack, T value) {
    auto [byteOffset, bitOffset] = divmod<field>();
    int bitWidth = detail::FieldTraits<field>::width;
    int bitsToPack = std::min(bitWidth, BYTE_SIZE - bitOffset);

    pack[byteOffset] |= ~U8_MASKS[bitsToPack]; // clear bits to write into
    pack[byteOffset] |= (static_cast<u8>(value) << bitOffset);

    for (int packedBits = bitsToPack; packedBits < bitWidth; ++byteOffset) {
      bitsToPack = std::min(bitWidth - packedBits, BYTE_SIZE);
      pack[byteOffset] &= ~U8_MASKS[bitsToPack]; // clear bits to write into
      pack[byteOffset] |= (static_cast<u8>(value >> packedBits));
      packedBits += bitsToPack;
    }
  }

  template<Field field, std::integral T>
  inline static T readBits(const std::array<u8, 12> &pack) {
    auto [byteOffset, bitOffset] = divmod<field>();
    int bitWidth = detail::FieldTraits<field>::width;

    T unpacked = static_cast<T>(pack[byteOffset]) >> bitOffset;

    for (int unpackedBits = BYTE_SIZE - bitOffset; unpackedBits < bitWidth; ++byteOffset) {
      int bitsToUnpack = std::min(bitWidth - unpackedBits, BYTE_SIZE);
      unpacked |= (static_cast<T>(pack[byteOffset] & U8_MASKS[bitsToUnpack]) << unpackedBits);
      unpackedBits += bitsToUnpack;
    }

    return unpacked;
  }

  template<typename... Ts, size_t... Is>
  inline static void insertBits(std::array<u8, 12> &pack, const std::tuple<Ts...> &tp, std::index_sequence<Is...>) {
    (writeBits<detail::IndexToField<Is>::field, Ts>(pack, std::get<Is>(tp)), ...);
  }

  std::array<u8, 12> pack_;

  template<Field field, typename T = int>
  [[nodiscard]] inline T getField() const {
    return readBits<field, T>(pack_);
  }

  template<Field field, typename T>
  requires std::is_integral_v<T> || std::is_enum_v<T>
  inline void setField(T value) {
    if constexpr(std::is_enum_v<T>) {
      using underlying = std::underlying_type_t<T>;
      writeBits<field, underlying>(pack_, static_cast<underlying>(value));
    } else {
      writeBits<field, T>(pack_, value);
    }
  }

public:
  PackedTTEntry() : pack_{} {}
  
  PackedTTEntry(u64 key, int score, int depth, Bound bound, std::pair<int, int> bestMove) : pack_{} {
    u32 vacantFlag = 1;
    u32 encodedMove = ((bestMove.first & 0x3f) << 6) | (bestMove.second & 0x3f);
    u32 boundValue = static_cast<int>(bound);
    u32 depthValue = depth;
    u32 scoreValue = score;
    u64 truncatedKey = key & U64_MASKS[detail::FieldTraits<Field::KEY>::width];

    auto options = std::make_tuple(vacantFlag, encodedMove, boundValue, depthValue, scoreValue, truncatedKey);
    insertBits(pack_, options, std::make_index_sequence<FIELD_COUNT>{});
  }
  
  [[nodiscard]] inline bool isOccupied() const {
    return ! getField<Field::VACANT>();
  }

  [[nodiscard]] inline bool hasMatchingKey(u64 otherKey) const {
    return getField<Field::KEY, u64>() == (otherKey & U64_MASKS[detail::FieldTraits<Field::KEY>::width]);
  }

  [[nodiscard]] inline Bound getBound() const {
    return Bound(getField<Field::BOUND>());
  }

  [[nodiscard]] inline int getEval() const {
    return getField<Field::SCORE>();
  }

  [[nodiscard]] inline bool hasAtLeastDepth(int depth) const {
    return getField<Field::DEPTH>() >= depth;
  }

  inline void setEval(int score) {
    setField<Field::SCORE>(score);
  }

  inline void setBound(Bound bound) {
    setField<Field::BOUND>(bound);
  }

  template<typename... FieldDataTypes>
  inline void update(FieldDataTypes... fds) {
    (setField<decltype(fds)::field_type, decltype(fds)::value_type>(fds.get()), ...);
  }

  inline void clear() {
    setField<Field::VACANT>(true);
  }
};

class TranspositionTable {
private:
  static constexpr size_t DEFAULT_HASH_SIZE = (1UL << 20) + 7;

  std::vector<PackedTTEntry> table_;
  size_t size_;

public:
  explicit TranspositionTable(size_t size = DEFAULT_HASH_SIZE) : table_{size}, size_{size} {}

  void store(u64 key, int score, int depth, Bound bound, std::pair<int,int> bestMove) {
    size_t index = key % size_;
    table_[index] = PackedTTEntry{key, score, depth, bound, bestMove};
  }

  std::optional<PackedTTEntry*> probe(u64 key) {
    size_t index = key % size_;
    auto& entry = table_[index];
    return (entry.isOccupied() && entry.hasMatchingKey(key)) ? std::optional<PackedTTEntry*>(&entry) : std::nullopt;
  }

  void clear() {
    for (auto& entry : table_)
      entry.clear();
  }
};
