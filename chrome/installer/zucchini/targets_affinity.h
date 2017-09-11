// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_
#define CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_

#include <stddef.h>

#include <vector>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class EquivalenceMap;

// Computes and stores affinity between old and new targets for a single target
// pool.
class TargetsAffinity {
 public:
  TargetsAffinity();
  ~TargetsAffinity();

  // Infers affinity between |old_targets| and |new_targets| using similarities
  // described by |equivalence_map|, and updates internal state for retrieval of
  // affinity metrics. Both |old_targets| and |new_targets| are targets in the
  // same pool and are sorted in ascending order.
  void InferFromSimilarities(const EquivalenceMap& equivalence_map,
                             const std::vector<offset_t>& old_targets,
                             const std::vector<offset_t>& new_targets);

  // Assigns labels to targets based on associations previously inferred.
  // Label 0 is assigned to targets that can not be associated are assigned.
  // Labels for old targets are written to |old_labels| and labels for new
  // targets are written to |new_labels|. Returns the upper bound on assigned
  // labels.
  size_t AssignLabels(double min_affinity,
                      std::vector<offset_t>* old_labels,
                      std::vector<offset_t>* new_labels);

  // Returns the affinity metric between targets identified by |old_keys| and
  // |new_keys|. Affinity > 0 means an association if more likely, < 0 means
  // incompatible association, and 0 means neither targets are associated.
  double AffinityBetween(offset_t old_keys, offset_t new_keys) const;

 private:
  struct Association {
    offset_t other = 0;
    double affinity = 0.0;
  };

  std::vector<Association> forward_association_;
  std::vector<Association> backward_association_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_
