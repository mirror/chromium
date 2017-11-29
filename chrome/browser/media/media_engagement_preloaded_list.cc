// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_engagement_preloaded_list.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/huffman_trie.h"
#include "base/path_service.h"
#include "chrome/browser/media/media_engagement_preload.pb.h"
#include "url/origin.h"

MediaEngagementPreloadedList::MediaEngagementPreloadedList() = default;

MediaEngagementPreloadedList::~MediaEngagementPreloadedList() = default;

bool MediaEngagementPreloadedList::CheckOriginIsPresent(url::Origin origin) {
  bool found = false;
  DCHECK(CheckStringIsPresent(origin.Serialize(), &found));
  return found;
}

static bool DecodeTrieMetadata(base::trie::BitReader* reader,
                               bool on_path,
                               size_t key_offset,
                               bool* out_found) {
  // There is no metadata embedded in the leaf nodes of the media-engagement
  // trie, thus nothing to parse.
  if (on_path) {
    DCHECK_EQ(0u, key_offset);
    *out_found = true;
  }

  return true;
}

bool MediaEngagementPreloadedList::CheckStringIsPresent(std::string input,
                                                        bool* out_found) {
  // Check that we have data to look through.
  if (!trie_bits_)
    return true;

  base::trie::Config config;
  config.trie_data = trie_data_.data();
  config.trie_bits = trie_bits_;
  config.root_position = root_position_;
  config.huffman_data = huffman_tree_.data();
  config.huffman_size = huffman_tree_.size();
  config.match_subdomains = false;

  return base::trie::Walk(config, input,
                          base::BindRepeating(DecodeTrieMetadata), out_found);
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
  return true;
}
