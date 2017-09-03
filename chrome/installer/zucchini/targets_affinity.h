// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_
#define CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_

#include <stddef.h>

#include <utility>
#include <vector>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class EquivalenceMap;

// Represents affinity between old and new targets.
class TargetsAffinity {
 public:
  TargetsAffinity();
  ~TargetsAffinity();

  // Infers affinity between |old_targets| and |new_targets| using similarities
  // described by |equivalence_map|, and updates internal state for retrieval of
  // affinity metrics.
  void InferFromSimilarities(const EquivalenceMap& equivalence_map,
                             const std::vector<offset_t>& old_targets,
                             const std::vector<offset_t>& new_targets);

  // Assigns labels to targets based on similarity previously inferred.
  // Labels for old targets are written to |old_labels| and labels for new
  // targets are written to |new_labels|. Returns the upper bound on assigned
  // labels.
  size_t AssignLabels(double min_affinity,
                      std::vector<size_t>* old_labels,
                      std::vector<size_t>* new_labels);

  // Returns the affinity metric between targets identified by |old_key| and
  // |new_key|.
  double AffinityBetween(offset_t old_key, offset_t new_key) const;

 private:
  struct Association {
    offset_t other;
    double affinity;
  };

  std::vector<Association> forward_affinity_;
  std::vector<Association> backward_affinity_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_
