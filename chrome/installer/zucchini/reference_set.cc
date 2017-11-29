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

ReferenceSet::ReferenceSet(const ReferenceTypeTraits& traits,
                           const TargetPool& target_pool)
    : traits_(traits), target_pool_(target_pool) {}
ReferenceSet::ReferenceSet(ReferenceSet&&) = default;
ReferenceSet::~ReferenceSet() = default;

void ReferenceSet::InitReferences(ReferenceReader&& ref_reader) {
  DCHECK(references_.empty());
  for (auto ref = ref_reader.GetNext(); ref.has_value();
       ref = ref_reader.GetNext()) {
    references_.push_back(
        {ref->location, target_pool_.KeyForOffset(ref->target)});
  }
}

void ReferenceSet::InitReferences(const std::vector<Reference>& refs) {
  DCHECK(references_.empty());
  DCHECK(std::is_sorted(refs.begin(), refs.end(),
                        [](const Reference& a, const Reference& b) {
                          return a.location < b.location;
                        }));
  std::transform(refs.begin(), refs.end(), std::back_inserter(references_),
                 [&](const Reference& ref) -> IndirectReference {
                   return {ref.location, target_pool_.KeyForOffset(ref.target)};
                 });
}

IndirectReference ReferenceSet::at(offset_t offset) const {
  auto pos = std::upper_bound(references_.begin(), references_.end(), offset,
                              [](offset_t a, const IndirectReference& ref) {
                                return a < ref.location;
                              });

  DCHECK(pos != references_.begin());
  --pos;
  DCHECK_LT(offset, pos->location + width());
  return *pos;
}

}  // namespace zucchini
