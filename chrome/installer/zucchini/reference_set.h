// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_REFERENCE_SET_H_
#define CHROME_INSTALLER_ZUCCHINI_REFERENCE_SET_H_

#include <stddef.h>

#include <vector>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class TargetPool;

// Container of indirect references for a specific type, along with its traits.
class ReferenceSet {
 public:
  using const_iterator = std::vector<IndirectReference>::const_iterator;

  // |traits| describes references that will be inserted. |target_pool| will be
  // use for indiction of reference targets and should contains all possible
  // targets.
  explicit ReferenceSet(const ReferenceTypeTraits& traits,
                        const TargetPool& target_pool);
  ReferenceSet(ReferenceSet&&);
  ReferenceSet(const ReferenceSet&) = delete;
  ~ReferenceSet();

  // Following methods insert to |*this|, all references read from |refs|.
  // This should be called exactly once.
  void InsertReferences(ReferenceReader&& refs);
  void InsertReferences(const std::vector<Reference>& refs);

  const std::vector<IndirectReference>& references() const {
    return references_;
  }
  const TargetPool& target_pool() const { return target_pool_; }
  const ReferenceTypeTraits& traits() const { return traits_; }
  TypeTag type_tag() const { return traits_.type_tag; }
  PoolTag pool_tag() const { return traits_.pool_tag; }
  offset_t width() const { return traits_.width; }

  // Assumes that a reference covers |location|, and returns the reference.
  IndirectReference at(offset_t location) const;

  size_t size() const { return references_.size(); }
  const_iterator begin() const { return references_.begin(); }
  const_iterator end() const { return references_.end(); }

 private:
  ReferenceTypeTraits traits_;
  const TargetPool& target_pool_;
  // List of Reference instances sorted by location (file offset).
  std::vector<IndirectReference> references_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_REFERENCE_SET_H_
