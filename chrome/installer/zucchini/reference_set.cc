// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/reference_set.h"

#include <algorithm>
#include <iterator>

#include "base/logging.h"
#include "base/macros.h"
#include "chrome/installer/zucchini/target_pool.h"

namespace zucchini {

ReferenceSet::ReferenceSet(const ReferenceTypeTraits& traits)
    : traits_(traits) {}
ReferenceSet::ReferenceSet(ReferenceSet&&) = default;
ReferenceSet::~ReferenceSet() = default;

void ReferenceSet::InsertReferences(zucchini::ReferenceReader&& ref_reader) {
  for (auto ref = ref_reader.GetNext(); ref.has_value();
       ref = ref_reader.GetNext()) {
    references_.push_back(*ref);
  }
}

void ReferenceSet::CanonicalizeTargets(
    const zucchini::TargetPool& target_pool) {
  for (auto& ref : references_) {
    ref.target = target_pool.KeyForOffset(ref.target);
  }
}

Reference ReferenceSet::at(offset_t location) const {
  auto pos = std::upper_bound(
      references_.begin(), references_.end(), location,
      [](offset_t a, const Reference& ref) { return a < ref.location; });

  DCHECK(pos != references_.begin());
  --pos;
  DCHECK_LT(location, pos->location + width());
  return *pos;
}

}  // namespace zucchini
