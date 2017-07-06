// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/labeled_pool.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"

namespace zucchini {

namespace {

// Sorts |offsets| and removes duplicates values.
void SortAndUniquify(std::vector<offset_t>* offsets) {
  std::sort(offsets->begin(), offsets->end());
  offsets->erase(std::unique(offsets->begin(), offsets->end()), offsets->end());
  offsets->shrink_to_fit();
}

}  // namespace

/******** LabeledPool ********/

LabeledPool::LabeledPool() = default;
LabeledPool::LabeledPool(LabeledPool&&) = default;
LabeledPool::~LabeledPool() = default;
LabeledPool& LabeledPool::operator=(LabeledPool&&) = default;

void LabeledPool::InsertTargets(const std::vector<Reference>& references) {
  DCHECK(labels_.empty());
  for (Reference reference : references)
    targets_.push_back(reference.target);
  SortAndUniquify(&targets_);
}

base::Optional<offset_t> LabeledPool::FindIndexForTarget(
    offset_t target) const {
  auto pos = std::lower_bound(targets_.begin(), targets_.end(), target);
  if (pos != targets_.end() && *pos == target)  // Checks if target is found.
    return static_cast<offset_t>(pos - targets_.begin());
  return base::nullopt;
}

offset_t LabeledPool::GetTargetForIndex(offset_t index) const {
  DCHECK(index < targets_.size());
  return targets_[index];
}

offset_t LabeledPool::GetLabelForIndex(offset_t index) const {
  DCHECK(index < labels_.size());
  return labels_[index];
}

void LabeledPool::ResetLabels(std::vector<offset_t>&& labels,
                              size_t label_bound) {
  DCHECK(labels.size() == targets_.size());
  labels_ = std::move(labels);
  label_bound_ = label_bound;
}

}  // namespace zucchini
