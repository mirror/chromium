// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/huffman_trie.h"

#include "base/callback.h"
#include "base/logging.h"

namespace base {

namespace trie {

// HuffmanDecoder is a simple Huffman reader. The input Huffman tree is encoded
// as a series of two-byte structures. The first byte determines the "0"
// pointer for that node and the second the "1" pointer. Each byte either has
// the MSB set, in which case the bottom 7 bits are the value for that
// position, or else the bottom seven bits contain the index of a node.
//
// The tree is decoded by walking rather than by using a table-driven approach.
//
// There is an implementation of the encoder here:
// net/tools/transport_security_state_generator/huffman/huffman_builder.h
class BASE_EXPORT HuffmanDecoder {
 public:
  HuffmanDecoder(const uint8_t* tree, size_t tree_bytes)
      : tree_(tree), tree_bytes_(tree_bytes) {}

  // Read the next bit from |reader| decode it and store the decoded char in
  // |out|.
  bool Decode(BitReader* reader, char* out) {
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

 private:
  const uint8_t* const tree_;
  const size_t tree_bytes_;
};

Config::Config() = default;

Config::~Config() = default;

bool Walk(const Config& config,
          const std::string& key,
          const RepeatingCallback<bool(BitReader*, bool, size_t, bool*)>&
              decode_metadata,
          bool* out_found) {
  HuffmanDecoder huffman(config.huffman_data, config.huffman_size);
  BitReader reader(config.trie_data, config.trie_bits);

  size_t bit_offset = config.root_position;
  static const char kEndOfString = 0;
  static const char kEndOfTable = 127;

  *out_found = false;

  // key_offset contains one more than the index of the current character
  // in the hostname that is being considered. It's one greater so that we can
  // represent the position just before the beginning (with zero).
  size_t key_offset = key.size();

  for (;;) {
    // Seek to the desired location.
    if (!reader.Seek(bit_offset)) {
      return false;
    }

    // Decode the unary length of the common prefix.
    size_t prefix_length;
    if (!reader.Unary(&prefix_length)) {
      return false;
    }

    // Match each character in the prefix.
    for (size_t i = 0; i < prefix_length; ++i) {
      if (key_offset == 0) {
        // We can't match the terminator with a prefix string.
        return true;
      }

      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }
      if (key[key_offset - 1] != c) {
        return true;
      }
      key_offset--;
    }

    bool is_first_offset = true;
    size_t current_offset = 0;

    // Next is the dispatch table.
    for (;;) {
      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }
      if (c == kEndOfTable) {
        // No exact match.
        return true;
      }

      if (c == kEndOfString) {
        const bool on_path = key_offset == 0 || (key[key_offset - 1] == '.' &&
                                                 config.match_subdomains);
        if (!decode_metadata.Run(&reader, on_path, key_offset, out_found)) {
          return false;
        }

        if (key_offset == 0) {
          return true;
        }

        continue;
      }

      // The entries in a dispatch table are in order thus we can tell if there
      // will be no match if the current character past the one that we want.
      if (key_offset == 0 || key[key_offset - 1] < c) {
        return true;
      }

      if (is_first_offset) {
        // The first offset is backwards from the current position.
        uint32_t jump_delta_bits;
        uint32_t jump_delta;
        if (!reader.Read(5, &jump_delta_bits) ||
            !reader.Read(jump_delta_bits, &jump_delta)) {
          return false;
        }

        if (bit_offset < jump_delta) {
          return false;
        }

        current_offset = bit_offset - jump_delta;
        is_first_offset = false;
      } else {
        // Subsequent offsets are forward from the target of the first offset.
        uint32_t is_long_jump;
        if (!reader.Read(1, &is_long_jump)) {
          return false;
        }

        uint32_t jump_delta;
        if (!is_long_jump) {
          if (!reader.Read(7, &jump_delta)) {
            return false;
          }
        } else {
          uint32_t jump_delta_bits;
          if (!reader.Read(4, &jump_delta_bits) ||
              !reader.Read(jump_delta_bits + 8, &jump_delta)) {
            return false;
          }
        }

        current_offset += jump_delta;
        if (current_offset >= bit_offset) {
          return false;
        }
      }

      DCHECK_LT(0u, key_offset);
      if (key[key_offset - 1] == c) {
        bit_offset = current_offset;
        key_offset--;
        break;
      }
    }
  }

  return true;
}

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

}  // namespace trie

}  // namespace base
