// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_MEDIA_ENGAGEMENT_PRELOAD_TRIE_WRITER_H_
#define TOOLS_MEDIA_ENGAGEMENT_PRELOAD_TRIE_WRITER_H_

#include <set>
#include <string>
#include <vector>

#include "net/tools/transport_security_state_generator/bit_writer.h"
#include "net/tools/transport_security_state_generator/huffman/huffman_builder.h"
#include "net/tools/transport_security_state_generator/trie/trie_bit_buffer.h"

namespace media {

typedef std::vector<uint8_t> Entry;
typedef std::vector<Entry> Entries;

class TrieWriter {
 public:
  enum : uint8_t { kTerminalValue = 0, kEndOfTableValue = 127 };

  TrieWriter(const net::transport_security_state::HuffmanRepresentationTable&
                 huffman_table,
             net::transport_security_state::HuffmanBuilder* huffman_builder);
  ~TrieWriter();

  // Constructs a trie containing all |entries|. The output is written to
  // |buffer_| and |*position| is set to the position of the trie root. Returns
  // true on success and false on failure.
  bool WriteEntries(const std::set<std::string>& entries, uint32_t* position);

  // Returns the position |buffer_| is currently at. The returned value
  // represents the number of bits.
  uint32_t position() const;

  // Flushes |buffer_|.
  void Flush();

  // Returns the trie bytes. Call Flush() first to ensure the buffer is
  // complete.
  const std::vector<uint8_t>& bytes() const { return buffer_.bytes(); }

 private:
  bool WriteDispatchTables(Entries::iterator start,
                           Entries::iterator end,
                           uint32_t* position);

  // Removes the first |length| characters from all entries between |start| and
  // |end|.
  void RemovePrefix(size_t length,
                    Entries::iterator start,
                    Entries::iterator end);

  // Searches for the longest common prefix for all entries between |start| and
  // |end|.
  std::vector<uint8_t> LongestCommonPrefix(Entries::const_iterator start,
                                           Entries::const_iterator end) const;

  // Returns the  |hostname| as a vector of bytes. The hostname will be
  // terminated by |kTerminalValue|.
  std::vector<uint8_t> ConvertNameToVector(const std::string& hostname) const;

  net::transport_security_state::BitWriter buffer_;
  const net::transport_security_state::HuffmanRepresentationTable&
      huffman_table_;
  net::transport_security_state::HuffmanBuilder* huffman_builder_;
};

}  // namespace media

#endif  // TOOLS_MEDIA_ENGAGEMENT_PRELOAD_TRIE_WRITER_H_
