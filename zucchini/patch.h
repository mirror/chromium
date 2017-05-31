// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_PATCH_H_
#define ZUCCHINI_PATCH_H_

#include <cstddef>
#include <cstdint>

#include "zucchini/image_utils.h"
#include "zucchini/stream.h"

namespace zucchini {

// "Copy offsets" are indexes into portions in "new" image that were copied from
// "old" image.
using copy_offset_t = offset_t;

struct RawDeltaUnit {
  copy_offset_t copy_offset;
  int8_t diff;
};

// Parameters for patch generation.
constexpr int kMinEquivalenceScore = 4;
constexpr int kLargeEquivalenceScore = 128;

// Enumeration for streams in the body of a patch.
struct PatchField {
  enum {
    // Command (1 stream).
    kCommand = 0,
    // Equivalence (3 streams).
    kSrcSkip = 1,
    kDstSkip = 2,
    kCopyCount = 3,
    // Extra Data (1 stream).
    kExtraData = 4,
    // Raw Delta (2 streams).
    kRawDeltaSkip = 5,
    kRawDeltaDiff = 6,
    // Reference Delta (1 stream).
    kReferenceDelta = 7,
    // Labels (variable number of streams). This must be the last enum; kLabels,
    // kLabels + 1, ... are used for different label pools.
    kLabels,
  };
};

// Constants that appear inside a patch.
enum PatchType : uint32_t {
  PATCH_TYPE_RAW = 0,
  PATCH_TYPE_SINGLE = 1,
  PATCH_TYPE_ENSEMBLE = 2,
  NUM_PATCH_TYPES
};

struct ZucchiniHeader {
  // Magic signature at the beginning of a Zucchini patch file. Since we encode
  // this as VarInt, so the actual bytes stored are: DA EA 8D 03.
  static constexpr uint32_t kMagic = 'Z' | ('u' << 8) | ('c' << 16);

  template <class Iterator>
  void WriteTo(SinkStream<Iterator>* sink) const {
    (*sink)(magic);
    (*sink)(old_size);
    (*sink)(old_crc);
    (*sink)(new_size);
    (*sink)(new_crc);
  }

  template <class Iterator>
  bool ReadFrom(SourceStream<Iterator>* src) {
    return (*src)(&magic) && (*src)(&old_size) && (*src)(&old_crc) &&
           (*src)(&new_size) && (*src)(&new_crc);
  }

  uint32_t magic = 0;
  uint32_t old_size = 0;
  uint32_t old_crc = 0;
  uint32_t new_size = 0;
  uint32_t new_crc = 0;
};

}  // namespace zucchini

#endif  // ZUCCHINI_PATCH_H_
