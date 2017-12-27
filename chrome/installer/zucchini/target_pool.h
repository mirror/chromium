// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TARGET_POOL_H_
#define CHROME_INSTALLER_ZUCCHINI_TARGET_POOL_H_

#include <stddef.h>

#include <vector>

#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/patch_reader.h"

namespace zucchini {

class EquivalenceMap;
class TargetSource;

// Ordered container of distinct targets that have the same semantics, along
// with a list of associated reference types.
class TargetPool {
 public:
  using const_iterator = std::vector<offset_t>::const_iterator;

  TargetPool();
  // Initializes the object with given sorted and unique |targets|.
  explicit TargetPool(std::vector<offset_t>&& targets);
  TargetPool(TargetPool&&);
  TargetPool(const TargetPool&);
  ~TargetPool();

  // The following functions insert each new target in |references|. This
  // invalidates all previous Key lookups.
  void InsertTargets(const std::vector<Reference>& references);
  void InsertTargets(ReferenceReader&& reader);
  void InsertTargets(TargetSource* targets);
  void InsertTargets(const std::vector<offset_t>& targets);

  // Adds |type| as a reference type associated with the pool of targets.
  void AddType(TypeTag type) { types_.push_back(type); }

  // Returns a canonical key associated with |offset|.
  key_t KeyForOffset(offset_t offset) const;

  // Returns the target for a given valid |key|, which is assumed to be
  // held by this class.
  offset_t OffsetForKey(key_t key) const { return targets_[key]; }

  void Project(const EquivalenceMap& equivalence_map);

  // Accessors for testing.
  const std::vector<offset_t>& targets() const { return targets_; }
  const std::vector<TypeTag>& types() const { return types_; }

  // Returns the number of targets.
  size_t size() const { return targets_.size(); }
  const_iterator begin() const;
  const_iterator end() const;

 private:
  std::vector<TypeTag> types_;     // Enumerates type_tag for this pool.
  std::vector<offset_t> targets_;  // Targets for pool in ascending order.
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TARGET_POOL_H_
