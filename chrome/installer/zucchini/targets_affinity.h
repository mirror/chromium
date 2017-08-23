// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_
#define CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_

#include <utility>
#include <vector>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class SimilarityMap;

class TargetsAffinity {
  struct Association {
    offset_t other;
    double affinity;
  };

 public:
  TargetsAffinity();
  ~TargetsAffinity();

  void InferFromSimilarities(
      const SimilarityMap& equivalence_map,
      const std::vector<offset_t>& old_targets,
      const std::vector<offset_t>& new_targets);

  size_t AssignLabels(std::vector<size_t>* old_labels,
                        std::vector<size_t>* new_labels);

  double AffinityBetween(offset_t old_index, offset_t new_index) const;

 private:
  std::vector<Association> forward_affinity_;
  std::vector<Association> backward_affinity_;
};

using TargetsAffinitySet = std::vector<TargetsAffinity>;

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TARGETS_AFFINITY_H_
