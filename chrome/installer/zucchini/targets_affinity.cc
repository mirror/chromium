// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/targets_affinity.h"

#include "base/logging.h"
#include "chrome/installer/zucchini/equivalence_map.h"

namespace zucchini {

TargetsAffinity::TargetsAffinity() = default;
TargetsAffinity::~TargetsAffinity() = default;

void TargetsAffinity::InferFromSimilarities(
    const SimilarityMap& equivalences,
    const std::vector<offset_t>& old_targets,
    const std::vector<offset_t>& new_targets) {
  forward_affinity_.assign(old_targets.size(), Association({0, 0.0}));
  backward_affinity_.assign(new_targets.size(), Association({0, 0.0}));
  offset_t new_base_index = 0;
  
  if (old_targets.empty() || new_targets.empty())
    return;

  for (auto candidate : equivalences) {
    // DCHECK(equivalence.dst_offset > old_targets[old_base_index]);
    for (; new_base_index != new_targets.size() &&
           new_targets[new_base_index] < candidate.eq.dst_offset;
         ++new_base_index) {
    }

    for (offset_t new_index = new_base_index;
         new_index != new_targets.size() &&
         new_targets[new_index] < candidate.eq.dst_offset + candidate.eq.length;
         ++new_index) {
      if (backward_affinity_[new_index].affinity >= candidate.similarity) {
        continue;
      }

      offset_t old_target = new_targets[new_index] - candidate.eq.dst_offset +
                            candidate.eq.src_offset;

      auto old_pos =
          std::lower_bound(old_targets.begin(), old_targets.end(), old_target);
      if (old_pos != old_targets.end()) {
        offset_t old_index =
            static_cast<offset_t>(old_pos - old_targets.begin());
        if (forward_affinity_[old_index].affinity < candidate.similarity) {
          if (forward_affinity_[old_index].affinity > 0.0) {
            backward_affinity_[forward_affinity_[old_index].other] = {0, 0.0};
          }
          if (backward_affinity_[new_index].affinity > 0.0) {
            forward_affinity_[backward_affinity_[new_index].other] = {0, 0.0};
          }
          forward_affinity_[old_index] = {new_index, candidate.similarity};
          backward_affinity_[new_index] = {old_index, candidate.similarity};
        }
      }
    }
  }
}

constexpr double kLabelMinAffinity = 64.0;

size_t TargetsAffinity::AssignLabels(std::vector<size_t>* old_labels,
                                       std::vector<size_t>* new_labels) {
  old_labels->assign(forward_affinity_.size(), 0);
  new_labels->assign(backward_affinity_.size(), 0);

  offset_t label = 1;
  for (offset_t old_index = 0; old_index < forward_affinity_.size();
       ++old_index) {
    Association association = forward_affinity_[old_index];
    if (association.affinity > kLabelMinAffinity) {
      (*old_labels)[old_index] = label;
      (*new_labels)[association.other] = label;
      ++label;
    }
  }
  return label;
}

double TargetsAffinity::AffinityBetween(offset_t old_index,
                                        offset_t new_index) const {
  DCHECK(old_index < forward_affinity_.size());
  DCHECK(new_index < backward_affinity_.size());
  if (forward_affinity_[old_index].other == new_index) {
    return forward_affinity_[old_index].affinity;
  }
  return -std::max(forward_affinity_[old_index].affinity,
                   backward_affinity_[new_index].affinity);
}

}  // namespace zucchini
