// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/huffman_decoder.h"

#include "base/logging.h"
#include "net/base/bit_reader.h"

namespace net {

HuffmanDecoder::HuffmanDecoder(const uint8_t* tree, size_t tree_bytes)
    : tree_(tree), tree_bytes_(tree_bytes) {}

bool HuffmanDecoder::Decode(BitReader* reader, char* out) {
  const uint8_t* current = &tree_[tree_bytes_ - 2];

  for (;;) {
    bool bit;
    if (!reader->Next(&bit))
      return false;

    uint8_t b = current[bit];
    if (b & 0x80) {
      *out = static_cast<char>(b & 0x7f);
      return true;
    }

    unsigned offset = static_cast<unsigned>(b) * 2;
    DCHECK_LT(offset, tree_bytes_);
    if (offset >= tree_bytes_)
      return false;

    current = &tree_[offset];
  }
}

}  // namespace net
