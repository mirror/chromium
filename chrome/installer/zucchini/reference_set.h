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

class ReferenceSet {
 public:
  using const_iterator = std::vector<IndirectReference>::const_iterator;

  explicit ReferenceSet(const ReferenceTypeTraits& traits);
  ReferenceSet(ReferenceSet&&);
  ReferenceSet(const ReferenceSet&) = delete;
  ~ReferenceSet();

  void InsertReferences(ReferenceReader&& ref_reader,
                        const TargetPool& target_pool);
  void InsertReferences(const std::vector<Reference>& refs,
                        const TargetPool& target_pool);

  const std::vector<IndirectReference>& references() const {
    return references_;
  }
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
  // List of Reference instances sorted by location (file offset).
  std::vector<IndirectReference> references_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_REFERENCE_SET_H_
