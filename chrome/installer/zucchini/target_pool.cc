// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/target_pool.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace zucchini {

TargetPool::TargetPool() = default;
TargetPool::TargetPool(TargetPool&&) = default;
TargetPool::~TargetPool() = default;

void TargetPool::Init(std::vector<offset_t>&& targets) {
  DCHECK(targets_.empty());
  DCHECK(std::is_sorted(targets.begin(), targets.end()));
  targets_ = std::move(targets);
}

void TargetPool::InsertTargets(const std::vector<Reference>& references) {
  std::transform(references.begin(), references.end(),
                 std::back_inserter(targets_),
                 [](const Reference& ref) { return ref.target; });
  std::sort(targets_.begin(), targets_.end());
  targets_.erase(std::unique(targets_.begin(), targets_.end()), targets_.end());
  targets_.shrink_to_fit();
}

offset_t TargetPool::KeyForOffset(offset_t target) const {
  auto pos = std::lower_bound(targets_.begin(), targets_.end(), target);
  DCHECK(pos != targets_.end() && *pos == target);
  return static_cast<offset_t>(pos - targets_.begin());
}

}  // namespace zucchini
