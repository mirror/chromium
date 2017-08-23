// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_index.h"

#include <algorithm>
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/equivalence_map.h"
#include "chrome/installer/zucchini/patch_reader.h"

namespace zucchini {

TargetPool::TargetPool() = default;
TargetPool::TargetPool(TargetPool&&) = default;
TargetPool::TargetPool(const TargetPool&) = default;
TargetPool::~TargetPool() = default;

void TargetPool::InsertTargets(const std::vector<Reference>& references) {
  std::transform(references.begin(), references.end(), std::back_inserter(targets_),
    [](const Reference& ref) { return ref.target; });
  std::sort(targets_.begin(), targets_.end());
  targets_.erase(std::unique(targets_.begin(), targets_.end()), targets_.end());
  targets_.shrink_to_fit();
}

void TargetPool::InsertTargets(ReferenceReader&& reader) {
  for (auto reference = reader.GetNext(); reference.has_value(); reference = reader.GetNext()) {
    targets_.push_back(reference->target);
  }
  std::sort(targets_.begin(), targets_.end());
  targets_.erase(std::unique(targets_.begin(), targets_.end()), targets_.end());
  targets_.shrink_to_fit();
}

void TargetPool::InsertTargets(TargetSource* targets) {
  for (auto target = targets->GetNext(); target.has_value(); target = targets->GetNext()) {
    targets_.push_back(*target);
  }
  std::sort(targets_.begin(), targets_.end());
}

void TargetPool::InsertTargets(const std::vector<offset_t>& targets) {
  std::copy(targets.begin(), targets.end(), std::back_inserter(targets_));
  std::sort(targets_.begin(), targets_.end());
}

void TargetPool::Project(const EquivalenceMap& equivalence_map) {
  equivalence_map.ProjectOffsets(&targets_);
  std::sort(targets_.begin(), targets_.end());
}

ReferenceSet::ReferenceSet(const ReferenceTypeTraits& traits)
    : traits_(traits) {}
  
ReferenceSet::ReferenceSet(ReferenceSet&&) = default;
ReferenceSet::~ReferenceSet() = default;

Reference ReferenceSet::FindReference(offset_t location) const {
  auto pos = std::upper_bound(
      references_.begin(),
      references_.end(), location,
      [](offset_t a, const Reference& ref) { return a < ref.location; });

  DCHECK(pos != references_.begin());
  --pos;
  DCHECK_LT(location, pos->location + width());
  return *pos;
}

void ReferenceSet::InsertReferences(zucchini::ReferenceReader&& ref_reader) {
  for (auto ref = ref_reader.GetNext(); ref.has_value(); ref = ref_reader.GetNext()) {
    references_.push_back(*ref);
  }
}

void ReferenceSet::CanonizeTargets(const zucchini::TargetPool& target_pool) {
  for (auto& ref : references_) {
    ref.target = target_pool.FindKeyForOffset(ref.target);
  }
}

ImageIndex::ImageIndex(ConstBufferView image)
    : raw_image_(image),
      type_tags_(image.size(), kNoTypeTag) {}

ImageIndex::~ImageIndex() = default;

bool ImageIndex::InsertReferences(Disassembler* disasm) {
  int pool_count = 0;
  std::vector<ReferenceGroup> ref_groups = disasm->MakeReferenceGroups();
  types_.reserve(ref_groups.size());
  for (const auto& group : ref_groups) {
    DCHECK_EQ(group.type_tag().value(), types_.size());
    types_.push_back(ReferenceSet(group.traits()));

    DCHECK(kNoPoolTag != group.pool_tag());
    pool_count = std::max(pool_count, group.pool_tag().value() + 1);
  }
  pools_.resize(pool_count);
  for (const auto& group : ref_groups) {
    pools_[group.pool_tag().value()].AddType(group.type_tag());
  }

  for (const auto& group : ref_groups) {
    //DCHECK(types_[group.type_tag().value()].empty());
    types_[group.type_tag().value()].InsertReferences(std::move(*group.GetReader(disasm)));
    for (auto ref : types_[group.type_tag().value()].references()) {
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
    pools_[group.pool_tag().value()].InsertTargets(types_[group.type_tag().value()].references());
  }
  for (auto& ref_set : types_) {
    const auto& target_pool = pools_[ref_set.pool_tag().value()];
    ref_set.CanonizeTargets(target_pool);
  }
  return true;
}

Reference ImageIndex::FindReference(TypeTag type, offset_t location) const {
  DCHECK_LE(location, size());
  DCHECK_LT(type.value(), types_.size());
  return types_[type.value()].FindReference(location);
}

bool ImageIndex::IsToken(offset_t location) const {
  TypeTag type = GetType(location);

  // |location| points into raw data.
  if (type == kNoTypeTag)
    return true;

  // |location| points into a Reference.
  Reference reference = FindReference(type, location);
  // Only the first byte of a reference is a token.
  return location == reference.location;
}

}  // namespace zucchini
