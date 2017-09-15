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

  for (auto candidate : equivalences) {  // Sorted by |dst_offset|.
    DCHECK_GT(candidate.similarity, 0.0);
    while (new_index < new_targets.size() &&
           new_targets[new_index] < candidate.eq.dst_offset) {
      ++new_index;
    }

    // Visit each new target covered by |candidate.eq| and find / update its
    // associated old target.
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
      // If new target can be mapped via |candidate.eq| to an old target, then
      // attempt to associate them. Multiple new targets can compete for the
      // same old target. The heuristic here makes selections to maximize
      // |candidate.similarity|, and if a tie occurs, minimize new target offset
      // (by first-come, first-served).
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
                                     std::vector<label_t>* old_labels,
                                     std::vector<label_t>* new_labels) {
  old_labels->assign(forward_association_.size(), 0);
  new_labels->assign(backward_association_.size(), 0);

  label_t label = 1;
  for (label_t old_index = 0; old_index < forward_association_.size();
       ++old_index) {
    Association association = forward_association_[old_index];
    if (association.affinity >= min_affinity) {
      (*old_labels)[old_index] = label;
      DCHECK_EQ(0U, (*new_labels)[association.other]);
      (*new_labels)[association.other] = label;
      ++label;
    }
  }
  return label;
}

double TargetsAffinity::AffinityBetween(offset_t old_key,
                                        offset_t new_key) const {
  DCHECK_LT(old_key, forward_association_.size());
  DCHECK_LT(new_key, backward_association_.size());
  if (forward_association_[old_key].affinity > 0.0 &&
      forward_association_[old_key].other == new_key) {
    DCHECK_EQ(backward_association_[new_key].other, old_key);
    DCHECK_EQ(forward_association_[old_key].affinity,
              backward_association_[new_key].affinity);
    return forward_association_[old_key].affinity;
  }
  return -std::max(forward_association_[old_key].affinity,
                   backward_association_[new_key].affinity);
}

}  // namespace zucchini
