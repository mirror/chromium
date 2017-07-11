// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_NUMERICS_SAFE_CONVERSIONS_ARM_IMPL_H_
#define BASE_NUMERICS_SAFE_CONVERSIONS_ARM_IMPL_H_

#include <cassert>
#include <limits>
#include <type_traits>

#include "base/numerics/safe_conversions_impl.h"

namespace base {
namespace internal {

template <typename T>
constexpr int ShiftWrapper(int shift) {
  return shift < std::numeric_limits<T>::digits ? shift : 1;
}

// This signed range comparison is faster on arm than the normal boundary
// checking due to the way arm handles fixed shift operations.
template <typename Dst, typename Src, typename Enable = void>
struct IsValueInRangeFastOp {
  static const bool is_supported = true;
  static bool Do(Src value) {
    assert(false);  // Should not be reached.
    return false;
  }
};

template <typename Dst, typename Src>
struct IsValueInRangeFastOp<
    Src,
    Dst,
    typename std::enable_if<
        std::is_integral<Dst>::value && std::is_integral<Src>::value &&
        std::is_signed<Dst>::value && std::is_signed<Src>::value &&
        IntegerBitsPlusSign<Src>::value <=
            IntegerBitsPlusSign<int32_t>::value &&
        !IsTypeInRangeForNumericType<Dst, Src>::value>::type> {
  static const bool is_supported = true;

  __attribute__((always_inline)) static constexpr bool Do(Src value) {
    return (value >> std::numeric_limits<Src>::digits) ==
           (value >> std::numeric_limits<Dst>::digits);
  }
};

// Fast saturation to a destination type.
template <typename Dst, typename Src>
struct SaturateFastAsmOp {
  static const bool is_supported =
      std::is_integral<Dst>::value && std::is_integral<Src>::value &&
      IntegerBitsPlusSign<Src>::value <= IntegerBitsPlusSign<int32_t>::value &&
      IntegerBitsPlusSign<Dst>::value <= IntegerBitsPlusSign<int32_t>::value &&
      !IsTypeInRangeForNumericType<Dst, Src>::value;

  __attribute__((always_inline)) static Dst Do(Src value) {
    typename std::conditional<std::is_signed<Src>::value, int32_t,
                              uint32_t>::type result = value;
    if (std::is_signed<Dst>::value) {
      if (!std::is_signed<Src>::value &&
          value > std::numeric_limits<Dst>::max()) {
        return std::numeric_limits<Dst>::max();
      }
      asm("ssat %[result], %[shift], %[result]"
          : [result] "+r"(result)
          :
          [shift] "n"(ShiftWrapper<int32_t>(std::numeric_limits<Dst>::digits)));
    } else {
      if (IsValueNegative(result))
        return Dst(0);
      asm("usat %[result], %[shift], %[result]"
          : [result] "+r"(result)
          : [shift] "n"(
              ShiftWrapper<uint32_t>(std::numeric_limits<Dst>::digits)));
    }
    return static_cast<Dst>(result);
  }
};

}  // namespace internal
}  // namespace base

#endif  // BASE_NUMERICS_SAFE_CONVERSIONS_ARM_IMPL_H_
