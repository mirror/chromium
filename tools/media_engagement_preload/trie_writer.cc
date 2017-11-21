// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/media_engagement_preload/trie_writer.h"

#include <algorithm>

#include "base/logging.h"
#include "net/tools/transport_security_state_generator/trie/trie_bit_buffer.h"

namespace {

bool CompareEntries(const media::Entry& lhs, const media::Entry& rhs) {
  return lhs < rhs;
}

}  // namespace

namespace media {

TrieWriter::TrieWriter(
    const net::transport_security_state::HuffmanRepresentationTable&
        huffman_table,
    net::transport_security_state::HuffmanBuilder* huffman_builder)
    : huffman_table_(huffman_table), huffman_builder_(huffman_builder) {}

TrieWriter::~TrieWriter() = default;

bool TrieWriter::WriteEntries(const std::set<std::string>& entries,
                              uint32_t* root_position) {
  if (entries.empty())
    return false;

  Entries processed_entries;
  for (auto const& entry : entries) {
    processed_entries.push_back(ConvertNameToVector(entry));
  }

  std::stable_sort(processed_entries.begin(), processed_entries.end(),
                   CompareEntries);

  return WriteDispatchTables(processed_entries.begin(), processed_entries.end(),
                             root_position);
}

bool TrieWriter::WriteDispatchTables(Entries::iterator start,
                                     Entries::iterator end,
                                     uint32_t* position) {
  DCHECK(start != end) << "No entries passed to WriteDispatchTables";

  net::transport_security_state::TrieBitBuffer writer;

  std::vector<uint8_t> prefix = LongestCommonPrefix(start, end);
  for (size_t i = 0; i < prefix.size(); ++i) {
    writer.WriteBit(1);
  }
  writer.WriteBit(0);

  if (prefix.size()) {
    for (size_t i = 0; i < prefix.size(); ++i) {
      writer.WriteChar(prefix.at(i), huffman_table_, huffman_builder_);
    }
  }

  RemovePrefix(prefix.size(), start, end);

  int32_t last_position = -1;

  while (start != end) {
    uint8_t candidate = (*start).at(0);
    Entries::iterator sub_entries_end = start + 1;

    for (; sub_entries_end != end; sub_entries_end++) {
      if ((*sub_entries_end).at(0) != candidate) {
        break;
      }
    }

    writer.WriteChar(candidate, huffman_table_, huffman_builder_);

    if (candidate == kTerminalValue) {
      if (sub_entries_end - start != 1) {
        return false;
      }
    } else {
      RemovePrefix(1, start, sub_entries_end);
      uint32_t table_position;
      if (!WriteDispatchTables(start, sub_entries_end, &table_position)) {
        return false;
      }

      writer.WritePosition(table_position, &last_position);
    }

    start = sub_entries_end;
  }

  writer.WriteChar(kEndOfTableValue, huffman_table_, huffman_builder_);

  *position = buffer_.position();
  writer.Flush();
  writer.WriteToBitWriter(&buffer_);
  return true;
}

void TrieWriter::RemovePrefix(size_t length,
                              Entries::iterator start,
                              Entries::iterator end) {
  for (Entries::iterator it = start; it != end; ++it) {
    it->erase(it->begin(), it->begin() + length);
  }
}

std::vector<uint8_t> TrieWriter::LongestCommonPrefix(
    Entries::const_iterator start,
    Entries::const_iterator end) const {
  if (start == end) {
    return std::vector<uint8_t>();
  }

  std::vector<uint8_t> prefix;
  for (size_t i = 0;; ++i) {
    if (i > start->size()) {
      break;
    }

    uint8_t candidate = (*start).at(i);
    if (candidate == kTerminalValue) {
      break;
    }

    bool ok = true;
    for (Entries::const_iterator it = start + 1; it != end; ++it) {
      if (i > it->size() || it->at(i) != candidate) {
        ok = false;
        break;
      }
    }

    if (!ok) {
      break;
    }

    prefix.push_back(candidate);
  }

  return prefix;
}

std::vector<uint8_t> TrieWriter::ConvertNameToVector(
    const std::string& hostname) const {
  size_t hostname_size = hostname.size();
  std::vector<uint8_t> name(hostname_size + 1);

  for (size_t i = 0; i < hostname_size; ++i) {
    name[i] = hostname[i];
  }

  name[name.size() - 1] = kTerminalValue;
  return name;
}

uint32_t TrieWriter::position() const {
  return buffer_.position();
}

void TrieWriter::Flush() {
  buffer_.Flush();
}

}  // namespace media
