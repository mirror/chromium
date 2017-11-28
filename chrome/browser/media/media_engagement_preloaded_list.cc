// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_preloaded_list.h"

#include "base/files/file_util.h"
#include "base/huffman_trie.h"
#include "base/memory/singleton.h"
#include "base/path_service.h"
#include "chrome/browser/media/media_engagement_preload.pb.h"
#include "url/origin.h"

namespace {

static const char kEndOfString = 0;
static const char kEndOfTable = 127;

}  // namespace

// static
MediaEngagementPreloadedList* MediaEngagementPreloadedList::GetInstance() {
  return base::Singleton<MediaEngagementPreloadedList>::get();
}

MediaEngagementPreloadedList::MediaEngagementPreloadedList() = default;

MediaEngagementPreloadedList::~MediaEngagementPreloadedList() = default;

bool MediaEngagementPreloadedList::CheckOriginIsPresent(url::Origin origin) {
  bool found = false;
  DCHECK(CheckStringIsPresent(origin.Serialize(), &found));
  return found;
}

bool MediaEngagementPreloadedList::CheckStringIsPresent(std::string input,
                                                        bool* out_found) {
  *out_found = false;

  // Check that we have data to look through.
  if (!IsLoaded())
    return true;

  base::TrieHuffmanDecoder huffman(huffman_tree_.data(), huffman_tree_.size());
  base::BitReader reader(trie_data_.data(), trie_bits_);
  size_t bit_offset = root_position_;

  size_t input_size = input.size();
  size_t input_offset = 0;

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
    for (size_t i = 0; i < prefix_length; i++) {
      if (input_offset == input_size) {
        // We can't match the terminator with a prefix string.
        return true;
      }

      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }
      if (input[input_offset] != c) {
        return true;
      }
      input_offset++;
    }

    bool is_first_offset = true;
    size_t current_offset = 0;

    // Go through the dispatch table. This contains the child nodes of the
    // current node in the trie.
    for (;;) {
      // Get the character for this node.
      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }

      // Reached the end of the dispatch table without finding a valid node.
      if (c == kEndOfTable) {
        return true;
      }

      // If we matched the end of the string then we should either return true
      // if we finished matching the string, or skip this node.
      if (c == kEndOfString) {
        if (input_offset == input_size) {
          *out_found = true;
          return true;
        }

        // There will be no pointer since this is the end of the string.
        continue;
      }

      // The entries in a dispatch table are in order thus we can tell if there
      // will be no match if the current character past the one that we want.
      if (input_offset == input_size || input[input_offset] < c) {
        return true;
      }

      // Read the offset to the next node in the dispatch table.
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

      // If the current character we are looking at matched this node then
      // we should move to this node and move to the next character.
      DCHECK_LT(input_offset, input_size);
      if (input[input_offset] == c) {
        bit_offset = current_offset;
        input_offset++;
        break;
      }
    }
  }

  return true;
}

bool MediaEngagementPreloadedList::LoadFromFile(base::FilePath path) {
  // Check the file exists.
  if (!base::PathExists(path))
    return false;

  // Read the file to a string.
  std::string file_data;
  if (!base::ReadFileToString(path, &file_data))
    return false;

  // Load the preloaded list into a proto message.
  chrome_browser_media::PreloadedData message;
  if (!message.ParseFromString(file_data))
    return false;

  // If any of the fields are empty we should return false.
  if (!message.has_huffman_tree() || !message.has_trie_data() ||
      !message.has_trie_bits() || !message.has_root_position()) {
    return false;
  }

  // Copy data into huffman tree.
  huffman_tree_ = std::vector<uint8_t>(message.huffman_tree().size());
  for (unsigned long i = 0; i < message.huffman_tree().size(); i++)
    huffman_tree_[i] = static_cast<uint8_t>(message.huffman_tree()[i]);

  // Copy data into trie data.
  trie_data_ = std::vector<uint8_t>(message.trie_data().size());
  for (unsigned long i = 0; i < message.trie_data().size(); i++)
    trie_data_[i] = static_cast<uint8_t>(message.trie_data()[i]);

  // Extract the final bits from the proto.
  trie_bits_ = message.trie_bits();
  root_position_ = message.root_position();

  is_loaded_ = true;

  if (on_load_closure_) {
    on_load_closure_->Run();
    on_load_closure_.reset();
  }

  return true;
}
