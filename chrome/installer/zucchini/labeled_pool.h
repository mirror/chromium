// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_LABELED_POOL_H_
#define CHROME_INSTALLER_ZUCCHINI_LABELED_POOL_H_

#include <cstddef>
#include <vector>

#include "base/optional.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// A Label is an alias for a Target, used to cluster them together.
// A LabeledPool is used to store a set of Targets forming a Pool, and their
// associated Labels. It provides functions to:
// - Retrieve an index for a given target.
// - Lookup the Target for a given index.
// - Lookup the Label for a given index.
// Targets are stored in an ordered vector and can be retrieved using binary
// search.
class LabeledPool {
 public:
  LabeledPool();
  LabeledPool(LabeledPool&&);
  ~LabeledPool();
  LabeledPool& operator=(LabeledPool&&);

  // Inserts every new targets found in references. This invalidates all
  // previous Index lookups. Labels are not assigned automatically and need to
  // be set with |ResetLabels|. Once labels are assigned, no additional targets
  // can be inserted.
  void InsertTargets(const std::vector<Reference>& references);

  // Returns true if |target| is found in targets.
  bool ContainsTarget(offset_t target) const;

  // Returns the index at which |target| is stored, or nullopt if |target| is
  // not found.
  base::Optional<offset_t> FindIndexForTarget(offset_t target) const;

  // Returns the target for a given valid |index|.
  offset_t GetTargetForIndex(offset_t index) const;

  // Returns the label for a given valid |index|. Labels must be assigned with
  // SwapLabels() first.
  offset_t GetLabelForIndex(offset_t index) const;

  // Given |labels|, a vector of N labels in the range [0, |label_bound|),
  // associates labels[index] to targets[index] for each index in [0..N), with N
  // the number of targets.
  void ResetLabels(std::vector<offset_t>&& labels, size_t label_bound);

  // Returns the number of targets.
  size_t size() const { return targets_.size(); }

  // Returns the ordered targets vector.
  const std::vector<offset_t>& targets() const { return targets_; }

  // Returns the labels vector.
  const std::vector<offset_t>& labels() const { return labels_; }

  // Returns the upper bound on labels.
  size_t label_bound() const { return label_bound_; }

 private:
  std::vector<offset_t> targets_;
  std::vector<offset_t> labels_;
  size_t label_bound_ = 0;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_LABELED_POOL_H_
