// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/region.h"

namespace zucchini {

// Minimalistic CRC-32 implementation for Zucchini usage. Adapted from LZMA SDK,
// which is public domain.
uint32_t Region::crc32() const {
  constexpr uint32_t kCrc32Poly = 0xEDB88320;
  static uint32_t kCrc32Table[256];

  static bool need_to_init_table = true;
  if (need_to_init_table) {
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t r = i;
      for (int j = 0; j < 8; ++j)
        r = (r >> 1) ^ (kCrc32Poly & ~((r & 1) - 1));
      kCrc32Table[i] = r;
    }
    need_to_init_table = false;
  }

  uint32_t ret = 0xFFFFFFFF;
  for (uint8_t v : *this)
    ret = kCrc32Table[(ret ^ v) & 0xFF] ^ (ret >> 8);
  return ret ^ 0xFFFFFFFF;
}

}  // namespace zucchini
