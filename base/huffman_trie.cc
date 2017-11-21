// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/huffman_trie.h"

#include "base/logging.h"

namespace base {

BitReader::BitReader(const uint8_t* bytes, size_t num_bits)
    : bytes_(bytes),
      num_bits_(num_bits),
      num_bytes_((num_bits + 7) / 8),
      current_byte_index_(0),
      num_bits_used_(8) {}

bool BitReader::Next(bool* out) {
  if (num_bits_used_ == 8) {
    if (current_byte_index_ >= num_bytes_)
      return false;

    current_byte_ = bytes_[current_byte_index_++];
    num_bits_used_ = 0;
  }

  *out = 1 & (current_byte_ >> (7 - num_bits_used_));
  num_bits_used_++;
  return true;
}

bool BitReader::Read(unsigned num_bits, uint32_t* out) {
  DCHECK_LE(num_bits, 32u);

  uint32_t ret = 0;
  for (unsigned i = 0; i < num_bits; ++i) {
    bool bit;
    if (!Next(&bit))
      return false;

    ret |= static_cast<uint32_t>(bit) << (num_bits - 1 - i);
  }

  *out = ret;
  return true;
}

bool BitReader::Unary(size_t* out) {
  size_t ret = 0;

  for (;;) {
    bool bit;
    if (!Next(&bit))
      return false;

    if (!bit)
      break;

    ret++;
  }

  *out = ret;
  return true;
}

bool BitReader::Seek(size_t offset) {
  if (offset >= num_bits_)
    return false;

  current_byte_index_ = offset / 8;
  current_byte_ = bytes_[current_byte_index_++];
  num_bits_used_ = offset % 8;
  return true;
}

TrieHuffmanDecoder::TrieHuffmanDecoder(const uint8_t* tree, size_t tree_bytes)
    : tree_(tree), tree_bytes_(tree_bytes) {}

bool TrieHuffmanDecoder::Decode(BitReader* reader, char* out) {
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

}  // namespace base
