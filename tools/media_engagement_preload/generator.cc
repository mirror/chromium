// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/media_engagement_preload/generator.h"

#include <stdint.h>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/media/media_engagement_preload.pb.h"
#include "net/tools/transport_security_state_generator/huffman/huffman_builder.h"
#include "tools/media_engagement_preload/trie_writer.h"

namespace media {

net::transport_security_state::HuffmanRepresentationTable ApproximateHuffman(
    const std::set<std::string>& entries) {
  net::transport_security_state::HuffmanBuilder huffman_builder;
  for (const auto& entry : entries) {
    for (const auto& c : entry)
      huffman_builder.RecordUsage(c);

    huffman_builder.RecordUsage(TrieWriter::kTerminalValue);
    huffman_builder.RecordUsage(TrieWriter::kEndOfTableValue);
  }

  return huffman_builder.ToTable();
}

bool ParseJSON(base::StringPiece json, std::set<std::string>* output) {
  std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
  base::ListValue* entries = nullptr;
  if (!value.get() || !value->GetAsList(&entries)) {
    LOG(ERROR) << "Could not parse the input JSON file";
    return false;
  }

  for (size_t i = 0; i < entries->GetSize(); i++) {
    std::string parsed;
    entries->GetString(i, &parsed);

    if (parsed.empty()) {
      LOG(ERROR) << "The hostname for entry " << base::SizeTToString(i)
                 << " is empty";
      return false;
    }

    output->insert(std::move(parsed));
  }

  return true;
}

std::string GenerateToProto(const std::set<std::string>& entries) {
  // The trie generation process is ran twice, the first time using an
  // approximate Huffman table. During this first run, the correct character
  // frequencies are collected which are then used to calculate the most space
  // efficient Huffman table for the given inputs. This table is used for the
  // second run.
  net::transport_security_state::HuffmanRepresentationTable table =
      ApproximateHuffman(entries);
  net::transport_security_state::HuffmanBuilder huffman_builder;
  TrieWriter writer(table, &huffman_builder);
  uint32_t root_position;
  if (!writer.WriteEntries(entries, &root_position))
    return std::string();

  net::transport_security_state::HuffmanRepresentationTable optimal_table =
      huffman_builder.ToTable();
  TrieWriter new_writer(optimal_table, nullptr);

  if (!new_writer.WriteEntries(entries, &root_position))
    return std::string();

  uint32_t new_length = new_writer.position();
  std::vector<uint8_t> huffman_tree = huffman_builder.ToVector();
  new_writer.Flush();

  chrome_browser_media::PreloadedData message;

  for (const uint8_t byte : huffman_tree)
    message.add_huffman_tree(static_cast<uint32_t>(byte));

  for (const uint8_t byte : new_writer.bytes())
    message.add_trie_data(static_cast<uint32_t>(byte));

  message.set_trie_bits(new_length);
  message.set_root_position(root_position);

  std::string output;
  message.SerializeToString(&output);
  return output;
}

}  // namespace media
