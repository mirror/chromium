// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HUFFMAN_TRIE_H_
#define BASE_HUFFMAN_TRIE_H_

#include <stdint.h>
#include <stdlib.h>

#include "base/base_export.h"
#include "base/callback_forward.h"
#include "base/strings/string_piece.h"

namespace base {

namespace trie {

class BitReader;

// Config represents a precomputed trie. The trie works over an alphabet of
// values in the range [1,126] and elements of that alphabet are Huffman
// compressed. Thus a complete trie consists of a pointer to the trie data
// itself, the offset (in bits) of the root of the trie, as well as a Huffman
// table.
//
// See net/tools/transport_security_state_generator/README.md for code to
// precompute suitable trie structures.
struct Config {
  Config();
  ~Config();

  const uint8_t* trie_data = nullptr;
  size_t trie_bits = 0;
  size_t root_position = 0;

  const uint8_t *huffman_data = nullptr;
  size_t huffman_size = 0;

  // match_subdomains indicates whether partial matches of a given key at a
  // DNS-label boundary should be considered to be “on path”. See the comment
  // for |Walk|.
  bool match_subdomains = false;
};

// Walk iterates over the trie specified by |config|, looking for |key|. It
// returns false if the trie data is found to be invalid and true otherwise.
//
// Leaf nodes in the trie contain embedded metadata which needs
// context-specific knowledge to parse. Thus a callback, |decode_metadata|,
// must be given that is capable of walking a |BitReader| over that metadata.
//
// The callback is also passed |on_path|, which indicates whether the current
// node is a partial or full match for the key, as opposed to being a node that
// needs to be skipped over. If |on_path| is true, the callback will likely
// want to save the parsed metadata. If |config.match_subdomains| is false,
// then |on_path| will only be true for an exact match. Otherwise it can be set
// if, for example, |key| is "foo.bar.com" and the metadata is for "bar.com".
// The callback can distinguish these cases by testing |key_offset|, which is
// one greater than the offset in |key| that is currently being matched. Thus,
// for an exact match, this will be zero.
//
// Lastly, if the callback considers the node to be acceptable, it can set
// |out_found|, which is equal to the final argument to |Walk|.
//
// If the callback returns false, |Walk| will abort and return false.
BASE_EXPORT bool Walk(
    const Config& config,
    const std::string& key,
    const RepeatingCallback<
        bool(BitReader*, bool on_path, size_t key_offset, bool* out_found)>&
        decode_metadata,
    bool* out_found);

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

}  // namespace trie

}  // namespace base

#endif  // BASE_HUFFMAN_TRIE_H_
