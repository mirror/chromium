// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_PRELOADED_LIST_H_
#define CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_PRELOADED_LIST_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace base {
class FilePath;
}  // namespace base

namespace url {
class Origin;
}  // namespace url

class MediaEngagementPreloadedList {
 public:
  MediaEngagementPreloadedList();
  ~MediaEngagementPreloadedList();

  // Load the contents from |path|.
  bool LoadFromFile(base::FilePath path);

  // Checks whether |origin| has a high global engagement and is present in the
  // preloaded list.
  bool CheckOriginIsPresent(url::Origin origin);

  // Check whether we have loaded a list.
  bool IsLoaded() const { return is_loaded_; }

  // Check whether the list we have loaded is empty.
  bool IsEmpty() const { return trie_bits_ == 0; }

 private:
  // Sets |result| to true if |input| is present in the list. If there was an
  // internal error when checking the list the function itself will return
  // false.
  bool CheckStringIsPresent(std::string input, bool* result);

  // The huffman tree.
  std::vector<uint8_t> huffman_tree_;

  // The trie data and the size of the trie.
  std::vector<uint8_t> trie_data_;
  size_t trie_bits_ = 0;

  // The root position of the trie.
  size_t root_position_;

  // If a list has been successfully loaded.
  bool is_loaded_ = false;

  DISALLOW_COPY_AND_ASSIGN(MediaEngagementPreloadedList);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_ENGAGEMENT_PRELOADED_LIST_H_
