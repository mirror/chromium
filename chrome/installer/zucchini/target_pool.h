// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_TARGET_POOL_H_
#define CHROME_INSTALLER_ZUCCHINI_TARGET_POOL_H_

#include <stddef.h>

#include <vector>

#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

class TargetPool {
 public:
  using const_iterator = std::vector<offset_t>::const_iterator;

  TargetPool();
  TargetPool(TargetPool&&);
  TargetPool(const TargetPool&) = delete;
  ~TargetPool();

  void Init(std::vector<offset_t>&& targets);

  // Inserts every new targets found in |references|. This invalidates all
  // previous Key lookups.
  void InsertTargets(const std::vector<Reference>& references);
  // Inserts every new targets found in |ref_reader|. This invalidates all
  // previous Key lookups.
  void InsertTargets(ReferenceReader&& ref_reader);

  // Adds |type| as a reference type associated with the pool of targets.
  void AddType(TypeTag type) { types_.push_back(type); }

  // Returns true if |target| is found in targets.
  bool ContainsTarget(offset_t target) const;

  // Returns a canonical key associated with |target|.
  key_t KeyForOffset(offset_t target) const;

  // Returns the target for a given valid |index|.
  offset_t OffsetForKey(key_t key) const { return targets_[key]; }

  // Returns the ordered targets vector.
  const std::vector<offset_t>& targets() const { return targets_; }

  // Returns the vector of reference types.
  const std::vector<TypeTag>& types() const { return types_; }

  // Returns the number of targets.
  size_t size() const { return targets_.size(); }
  const_iterator begin() const;
  const_iterator end() const;

 private:
  std::vector<offset_t> targets_;
  std::vector<TypeTag> types_;  // Enumerates type_tag for this pool.
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_TARGET_POOL_H_
