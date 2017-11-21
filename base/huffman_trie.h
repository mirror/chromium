// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HUFFMAN_TRIE_H_
#define BASE_HUFFMAN_TRIE_H_

#include <stdint.h>
#include <stdlib.h>

#include "base/base_export.h"

namespace base {

// BitReader is a class that allows a bytestring to be read bit-by-bit.
class BASE_EXPORT BitReader {
 public:
  BitReader(const uint8_t* bytes, size_t num_bits);

  // Next sets |*out| to the next bit from the input. It returns false if no
  // more bits are available or true otherwise.
  bool Next(bool* out);

  // Read sets the |num_bits| least-significant bits of |*out| to the value of
  // the next |num_bits| bits from the input. It returns false if there are
  // insufficient bits in the input or true otherwise.
  bool Read(unsigned num_bits, uint32_t* out);

  // Unary sets |*out| to the result of decoding a unary value from the input.
  // It returns false if there were insufficient bits in the input and true
  // otherwise.
  bool Unary(size_t* out);

  // Seek sets the current offest in the input to bit number |offset|. It
  // returns true if |offset| is within the range of the input and false
  // otherwise.
  bool Seek(size_t offset);

 private:
  const uint8_t* const bytes_;
  const size_t num_bits_;
  const size_t num_bytes_;
  // current_byte_index_ contains the current byte offset in |bytes_|.
  size_t current_byte_index_;
  // current_byte_ contains the current byte of the input.
  uint8_t current_byte_;
  // num_bits_used_ contains the number of bits of |current_byte_| that have
  // been read.
  unsigned num_bits_used_;
};

// HuffmanDecoder is a very simple Huffman reader. The input Huffman tree is
// simply encoded as a series of two-byte structures. The first byte determines
// the "0" pointer for that node and the second the "1" pointer. Each byte
// either has the MSB set, in which case the bottom 7 bits are the value for
// that position, or else the bottom seven bits contain the index of a node.
//
// The tree is decoded by walking rather than a table-driven approach.
//
// There is an implementation of the encoder here:
// net/tools/transport_security_state_generator/huffman/huffman_builder.h
//
// Usage guidelines:
//  - Generate the Huffman Tree using the encoder.
//  - Pass the generated Huffman Tree and the size in bytes to the constructor.
//  - Create a BitReader instance with your huffman compressed trie.
//    (spec is here: net/tools/transport_security_state_generator/README.md)
//  - Call the decode function to read the next bit and decode it.
class BASE_EXPORT TrieHuffmanDecoder {
 public:
  TrieHuffmanDecoder(const uint8_t* tree, size_t tree_bytes);

  // Read the next bit from |reader| decode it and store the decoded char in
  // |out|.
  bool Decode(BitReader* reader, char* out);

 private:
  const uint8_t* const tree_;
  const size_t tree_bytes_;
};

}  // namespace base

#endif  // BASE_HUFFMAN_TRIE_H_
