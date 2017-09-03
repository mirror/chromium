// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_index.h"

#include <algorithm>
#include <utility>

#include "base/macros.h"
#include "base/stl_util.h"
#include "chrome/installer/zucchini/disassembler.h"

namespace zucchini {

ImageIndex::ImageIndex(ConstBufferView image)
    : image_(image), type_tags_(image.size(), kNoTypeTag) {}
ImageIndex::ImageIndex(ImageIndex&&) = default;
ImageIndex::~ImageIndex() = default;

bool ImageIndex::Initialize(Disassembler* disasm) {
  std::vector<ReferenceGroup> ref_groups = disasm->MakeReferenceGroups();

  for (const auto& group : ref_groups) {
    DCHECK(kNoTypeTag != group.type_tag());
    auto result = reference_sets_.emplace(group.type_tag(), group.traits());
    DCHECK(result.second);
  }
  for (const auto& group : ref_groups) {
    DCHECK(kNoPoolTag != group.pool_tag());
    auto pos = target_pools_.find(group.pool_tag());
    if (pos == target_pools_.end())
      pos = target_pools_.emplace(group.pool_tag(), TargetPool()).first;
    pos->second.AddType(group.type_tag());
  }

  for (const auto& group : ref_groups) {
    reference_sets_.at(group.type_tag())
        .InsertReferences(std::move(*group.GetReader(disasm)));
    for (auto ref : reference_sets_.at(group.type_tag())) {
      DCHECK_LE(ref.location + group.width(), size());

      // Check for overlap with existing reference. If found, then invalidate.
      if (std::any_of(type_tags_.begin() + ref.location,
                      type_tags_.begin() + ref.location + group.width(),
                      [](TypeTag type) { return type != kNoTypeTag; }))
        return false;
      std::fill(type_tags_.begin() + ref.location,
                type_tags_.begin() + ref.location + group.width(),
                group.type_tag());
    }
    target_pools_.at(group.pool_tag())
        .InsertTargets(reference_sets_.at(group.type_tag()).references());
  }
  for (auto& ref_set : reference_sets_) {
    const auto& target_pool = target_pools_.at(ref_set.second.pool_tag());
    ref_set.second.CanonicalizeTargets(target_pool);
  }
  return true;
}

bool ImageIndex::IsToken(offset_t location) const {
  TypeTag type = LookupType(location);

  // |location| points into raw data.
  if (type == kNoTypeTag)
    return true;

  // |location| points into a Reference.
  Reference reference = at(type).at(location);
  // Only the first byte of a reference is a token.
  return location == reference.location;
}

}  // namespace zucchini
