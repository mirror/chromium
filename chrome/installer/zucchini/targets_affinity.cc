// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/targets_affinity.h"

#include <algorithm>

#include "base/logging.h"
#include "chrome/installer/zucchini/equivalence_map.h"

namespace zucchini {

TargetsAffinity::TargetsAffinity() = default;
TargetsAffinity::~TargetsAffinity() = default;

void TargetsAffinity::InferFromSimilarities(
    const EquivalenceMap& equivalences,
    const std::vector<offset_t>& old_targets,
    const std::vector<offset_t>& new_targets) {
  forward_association_.assign(old_targets.size(), {});
  backward_association_.assign(new_targets.size(), {});
  offset_t new_index = 0;

  if (old_targets.empty() || new_targets.empty())
    return;

  for (auto candidate : equivalences) {
    DCHECK_GT(candidate.similarity, 0.0);
    while (new_index < new_targets.size() &&
           new_targets[new_index] < candidate.eq.dst_offset) {
      ++new_index;
    }

    for (; new_index < new_targets.size() &&
           new_targets[new_index] < candidate.eq.dst_end();
         ++new_index) {
      DCHECK_GE(new_targets[new_index], candidate.eq.dst_offset);
      if (backward_association_[new_index].affinity >= candidate.similarity)
        continue;

      offset_t old_target = new_targets[new_index] - candidate.eq.dst_offset +
                            candidate.eq.src_offset;

      auto old_pos =
          std::lower_bound(old_targets.begin(), old_targets.end(), old_target);
      if (old_pos != old_targets.end() && *old_pos == old_target) {
        offset_t old_index =
            static_cast<offset_t>(old_pos - old_targets.begin());
        if (candidate.similarity > forward_association_[old_index].affinity) {
          // Reset other associations.
          if (forward_association_[old_index].affinity > 0.0)
            backward_association_[forward_association_[old_index].other] = {};
          if (backward_association_[new_index].affinity > 0.0)
            forward_association_[backward_association_[new_index].other] = {};
          // Assign new association.
          forward_association_[old_index] = {new_index, candidate.similarity};
          backward_association_[new_index] = {old_index, candidate.similarity};
        }
      }
    }
  }
}

size_t TargetsAffinity::AssignLabels(double min_affinity,
                                     std::vector<offset_t>* old_labels,
                                     std::vector<offset_t>* new_labels) {
  old_labels->assign(forward_association_.size(), 0);
  new_labels->assign(backward_association_.size(), 0);

  offset_t label = 1;
  for (offset_t old_index = 0; old_index < forward_association_.size();
       ++old_index) {
    Association association = forward_association_[old_index];
    if (association.affinity >= min_affinity) {
      (*old_labels)[old_index] = label;
      (*new_labels)[association.other] = label;
      ++label;
    }
  }
  return label;
}

double TargetsAffinity::AffinityBetween(offset_t old_keys,
                                        offset_t new_keys) const {
  DCHECK_LT(old_keys, forward_association_.size());
  DCHECK_LT(new_keys, backward_association_.size());
  if (forward_association_[old_keys].affinity > 0.0 &&
      forward_association_[old_keys].other == new_keys) {
    DCHECK_EQ(backward_association_[new_keys].other, old_keys);
    DCHECK_EQ(forward_association_[old_keys].affinity,
              backward_association_[new_keys].affinity);
    return forward_association_[old_keys].affinity;
  }
  return -std::max(forward_association_[old_keys].affinity,
                   backward_association_[new_keys].affinity);
}

}  // namespace zucchini
