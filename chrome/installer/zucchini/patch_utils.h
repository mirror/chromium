// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_PATCH_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_PATCH_UTILS_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

struct RawDeltaUnit {
  offset_t copy_offset;
  int8_t diff;
};

// Constants that appear inside a patch.
enum class PatchType : uint32_t {
  kRawPatch = 0,
  kSinglePatch = 1,
  kEnsemblePatch = 2,
  kCount,
  kUnrecognisedPatch,
};

#pragma pack(push, 1)  // Supported by MSVC and GCC. Ensures no gaps in packing.
struct PatchHeader {
  // Magic signature at the beginning of a Zucchini patch file.
  static constexpr uint32_t kMagic = 'Z' | ('u' << 8) | ('c' << 16);

  uint32_t magic = 0;
  uint32_t old_size = 0;
  uint32_t old_crc = 0;
  uint32_t new_size = 0;
  uint32_t new_crc = 0;
  PatchType patch_type;
};

struct ElementPatchHeader {
  uint32_t old_offset;
  uint32_t new_offset;
  uint64_t old_length;
  uint64_t new_length;
  ExecutableType exe_type;
};
#pragma pack(pop)

template <class T, class It>
It EncodeVarUInt(T value, It dst) {
  static_assert(std::is_unsigned<T>::value, "Value type must be unsigned");
  while (value >= 128) {
    *dst++ = static_cast<uint8_t>(value) | 128;
    value >>= 7;
  }
  *dst++ = static_cast<uint8_t>(value);
  return dst;
}

template <class T, class It>
It EncodeVarInt(T value, It dst) {
  static_assert(std::is_signed<T>::value, "Value type must be signed");

  using unsigned_value_type = typename std::make_unsigned<T>::type;
  if (value < 0)
    return EncodeVarUInt(unsigned_value_type(~value * 2 + 1), dst);
  else
    return EncodeVarUInt(unsigned_value_type(value * 2), dst);
}

template <class T, class It>
base::Optional<It> DecodeVarUInt(It first, It last, T* value) {
  static_assert(std::is_unsigned<T>::value, "Value type must be unsigned");

  uint8_t sh = 0;
  T val = 0;
  while (first != last) {
    val |= T(*first & 0x7F) << sh;
    if (*(first++) < 0x80) {
      if (sh > sizeof(T) * 8)  // Overflow!
        return base::nullopt;
      // Forgive overflow from the last byte.
      *value = val;
      return first;
    }
    sh += 7;
  }
  return base::nullopt;
}

template <class T, class It>
base::Optional<It> DecodeVarInt(It first, It last, T* value) {
  static_assert(std::is_signed<T>::value, "Value type must be signed");

  typename std::make_unsigned<T>::type tmp = 0;
  auto res = DecodeVarUInt(first, last, &tmp);
  if (!res)
    return res;
  if (tmp & 1)
    *value = ~static_cast<T>(tmp >> 1);
  else
    *value = static_cast<T>(tmp >> 1);
  return res;
}

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_PATCH_UTILS_H_
