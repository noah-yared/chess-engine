#pragma once

#include <cstdint>

#if defined(UINT64_MAX) && defined(UINT32_MAX) && defined(INT16_MAX) && defined(UINT16_MAX) && defined(UINT8_MAX)
  using u64 = uint64_t;
  using u32 = uint32_t;
  using i16 = int16_t;
  using u16 = uint16_t;
  using u8 = uint8_t;
#else
  #error "Unsupported platform/architecture: No 64,32,16,8-bit unsigned or 16-bit signed integer type found."
#endif

#if defined(__GNUC__) || defined(__clang__)
// compiler hints
// #define LIKELY(cond) __builtin_expect(!!(cond), 1)
#define LIKELY(cond) [[likely]] (cond)
// #define UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#define UNLIKELY(cond) [[unlikely]] (cond)
#else
#define LIKELY(cond) (cond)
#define UNLIKELY(cond) (cond)
#endif
