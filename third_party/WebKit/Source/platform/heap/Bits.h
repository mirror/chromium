// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace blink {

namespace bits {

// CountPopulation(value) returns the number of bits set in |value|.
template <typename T>
constexpr inline
    typename std::enable_if<std::is_unsigned<T>::value && sizeof(T) <= 8,
                            unsigned>::type
    CountPopulation(T value) {
#ifdef __GNUC__
  return sizeof(T) == 8 ? __builtin_popcountll(static_cast<uint64_t>(value))
                        : __builtin_popcount(static_cast<uint32_t>(value));
#else
  constexpr uint64_t mask[] = {0x5555555555555555, 0x3333333333333333,
                               0x0f0f0f0f0f0f0f0f, 0x00ff00ff00ff00ff,
                               0x0000ffff0000ffff, 0x00000000ffffffff};
  value = ((value >> 1) & mask[0]) + (value & mask[0]);
  value = ((value >> 2) & mask[1]) + (value & mask[1]);
  value = ((value >> 4) & mask[2]) + (value & mask[2]);
  if (sizeof(T) > 1)
    value = ((value >> (sizeof(T) > 1 ? 8 : 0)) & mask[3]) + (value & mask[3]);
  if (sizeof(T) > 2)
    value = ((value >> (sizeof(T) > 2 ? 16 : 0)) & mask[4]) + (value & mask[4]);
  if (sizeof(T) > 4)
    value = ((value >> (sizeof(T) > 4 ? 32 : 0)) & mask[5]) + (value & mask[5]);
  return static_cast<unsigned>(value);
#endif
}

// CountTrailingZeros(value) returns the number of zero bits preceding the
// least significant 1 bit in |value| if |value| is non-zero, otherwise it
// returns {sizeof(T) * 8}.
template <typename T, unsigned bits = sizeof(T) * 8>
inline constexpr
    typename std::enable_if<std::is_integral<T>::value && sizeof(T) <= 8,
                            unsigned>::type
    CountTrailingZeros(T value) {
#ifdef __GNUC__
  return value == 0 ? bits
                    : bits == 64 ? __builtin_ctzll(static_cast<uint64_t>(value))
                                 : __builtin_ctz(static_cast<uint32_t>(value));
#else
  // Fall back to popcount (see "Hacker's Delight" by Henry S. Warren, Jr.),
  // chapter 5-4. On x64, since is faster than counting in a loop and faster
  // than doing binary search.
  using U = typename std::make_unsigned<T>::type;
  U u = value;
  return CountPopulation(static_cast<U>(~u & (u - 1u)));
#endif
}

// CountLeadingZeros(value) returns the number of zero bits following the most
// significant 1 bit in |value| if |value| is non-zero, otherwise it returns
// {sizeof(T) * 8}.
template <typename T, unsigned bits = sizeof(T) * 8>
inline constexpr
    typename std::enable_if<std::is_unsigned<T>::value && sizeof(T) <= 8,
                            unsigned>::type
    CountLeadingZeros(T value) {
  static_assert(bits > 0, "invalid instantiation");
#if __GNUC__
  return value == 0
             ? bits
             : bits == 64
                   ? __builtin_clzll(static_cast<uint64_t>(value))
                   : __builtin_clz(static_cast<uint32_t>(value)) - (32 - bits);
#else
  // Binary search algorithm taken from "Hacker's Delight" (by Henry S. Warren,
  // Jr.), figures 5-11 and 5-12.
  if (bits == 1)
    return static_cast<unsigned>(value) ^ 1;
  T upper_half = value >> (bits / 2);
  T next_value = upper_half != 0 ? upper_half : value;
  unsigned add = upper_half != 0 ? 0 : bits / 2;
  constexpr unsigned next_bits = bits == 1 ? 1 : bits / 2;
  return CountLeadingZeros<T, next_bits>(next_value) + add;
#endif
}

}  // namespace bits

}  // namespace blink
