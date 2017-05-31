// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/label_manager.h"

namespace zucchini {

BaseLabelManager::BaseLabelManager() = default;
BaseLabelManager::~BaseLabelManager() = default;

void OrderedLabelManager::Assign(offset_t* ref) const {
  if (labels_.empty())
    return;
  if (IsMarked(*ref))
    return;
  auto it = ranges::lower_bound(labels_, *ref);
  if (it != labels_.end() && *it == *ref) {
    *ref = MarkIndex(it - labels_.begin());
  }
}

UnorderedLabelManager::UnorderedLabelManager() = default;
UnorderedLabelManager::~UnorderedLabelManager() = default;

void UnorderedLabelManager::Assign(offset_t* ref) {
  if (IsMarked(*ref))
    return;
  if (labels_.empty())
    return;
  UpdateMap();
  auto it = labels_map_.find(*ref);
  if (it != labels_map_.end()) {
    *ref = MarkIndex(it->second);
  }
}

void UnorderedLabelManager::clear() {
  labels_.clear();
  labels_map_.clear();
  first_unindexed_label_ = 0;
  first_unused_idx_ = 0;
}

void UnorderedLabelManager::UpdateMap() {
  for (; first_unindexed_label_ < labels_.size(); ++first_unindexed_label_) {
    if (labels_[first_unindexed_label_] == kUnusedIndex)
      continue;
    // labels_map_.emplace(labels_[first_unindexed_label_],
    //                     offset_t(first_unindexed_label_));
    labels_map_[labels_[first_unindexed_label_]] =
        offset_t(first_unindexed_label_);
  }
}

void UnorderedLabelManager::InPlaceDigest() {
  // All labels in |labels_| starting from |first_unindexed_label_| are assumed
  // to be different from |kUnusedIndex|.
  auto it = labels_.begin() + first_unindexed_label_;
  while (it != labels_.end()) {
    assert(*it != kUnusedIndex);
    if (first_unused_idx_ >= first_unindexed_label_ ||
        labels_[first_unused_idx_] == kUnusedIndex) {
      labels_[first_unused_idx_] = *it;
      // labels_map_.emplace(*it, offset_t(first_unused_idx_));
      labels_map_[*it] = offset_t(first_unused_idx_);
      ++it;
    }
    ++first_unused_idx_;
  }
  labels_.resize(std::max(first_unused_idx_, first_unindexed_label_));
  first_unindexed_label_ = labels_.size();
}

}  // namespace zucchini
