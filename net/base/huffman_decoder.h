// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_HUFFMAN_DECODER_H_
#define NET_BASE_HUFFMAN_DECODER_H_

#include <stdint.h>
#include <stdlib.h>

#include "net/base/net_export.h"

namespace net {

class BitReader;

// HuffmanDecoder is a very simple Huffman reader. The input Huffman tree is
// simply encoded as a series of two-byte structures. The first byte determines
// the "0" pointer for that node and the second the "1" pointer. Each byte
// either has the MSB set, in which case the bottom 7 bits are the value for
// that position, or else the bottom seven bits contain the index of a node.
//
// The tree is decoded by walking rather than a table-driven approach.
class NET_EXPORT HuffmanDecoder {
 public:
  HuffmanDecoder(const uint8_t* tree, size_t tree_bytes);

  // Read the next bit from |reader| decode it and store the decoded char in
  // |out|.
  bool Decode(BitReader* reader, char* out);

 private:
  const uint8_t* const tree_;
  const size_t tree_bytes_;
};

}  // namespace net

#endif  // NET_BASE_HUFFMAN_DECODER_H_
