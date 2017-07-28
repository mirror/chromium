// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_index.h"

#include <algorithm>

#include "chrome/installer/zucchini/disassembler.h"

namespace zucchini {

ImageIndex::ImageIndex(ConstBufferView image,
                       std::vector<ReferenceTypeTraits>&& traits_map)
    : raw_image_(image),
      traits_(traits_map),
      references_(traits_.size()),
      type_tags_(image.size(), kNoTypeTag) {
  int pool_count = 0;
  for (const auto& traits : traits_) {
    DCHECK(kNoPoolTag != traits.pool_tag);
    pool_count = std::max(pool_count, traits.pool_tag.value() + 1);
  }
  pool_types_.resize(pool_count);
  label_count_.resize(pool_count, 0);
  for (const auto& traits : traits_) {
    pool_types_[traits.pool_tag.value()].push_back(traits.type_tag);
  }
}

ImageIndex::~ImageIndex() = default;

bool ImageIndex::InsertReferences(TypeTag type, ReferenceReader&& ref_reader) {
  const ReferenceTypeTraits& traits = GetTraits(type);
  for (base::Optional<Reference> ref = ref_reader.GetNext(); ref.has_value();
       ref = ref_reader.GetNext()) {
    DCHECK_LE(ref->location + traits.width, size());

    // Check for overlap with existing reference. If found, then invalidate.
    if (std::any_of(type_tags_.begin() + ref->location,
                    type_tags_.begin() + ref->location + traits.width,
                    [](TypeTag type) { return type != kNoTypeTag; }))
      return false;
    std::fill(type_tags_.begin() + ref->location,
              type_tags_.begin() + ref->location + traits.width,
              traits.type_tag);
    references_[traits.type_tag.value()].push_back(*ref);
  }
  return true;
}

Reference ImageIndex::FindReference(TypeTag type, offset_t location) const {
  DCHECK_LE(location, size());
  DCHECK_LT(type.value(), references_.size());
  auto pos = std::upper_bound(
      references_[type.value()].begin(), references_[type.value()].end(),
      location,
      [](offset_t a, const Reference& ref) { return a < ref.location; });

  DCHECK(pos != references_[type.value()].begin());
  --pos;
  DCHECK_LT(location, pos->location + GetTraits(type).width);
  return *pos;
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

void ImageIndex::LabelTargets(PoolTag pool,
                              const BaseLabelManager& label_manager) {
  for (const auto& type : pool_types_[pool.value()])
    for (auto& ref : references_[type.value()])
      ref.target = label_manager.MarkedIndexFromOffset(ref.target);
  label_count_[pool.value()] = label_manager.size();
}

void ImageIndex::UnlabelTargets(PoolTag pool,
                                const BaseLabelManager& label_manager) {
  for (const auto& type : pool_types_[pool.value()])
    for (auto& ref : references_[type.value()])
      ref.target = label_manager.OffsetFromMarkedIndex(ref.target);
  label_count_[pool.value()] = 0;
}

void ImageIndex::LabelAssociatedTargets(
    PoolTag pool,
    const BaseLabelManager& label_manager,
    const BaseLabelManager& reference_label_manager) {
  // Convert to marked indexes.
  for (const auto& type : pool_types_[pool.value()]) {
    for (auto& ref : references_[type.value()]) {
      // Represent Label as marked index iff the index is also in
      // |reference_label_manager|.
      DCHECK(!IsMarked(ref.target));
      offset_t index = label_manager.IndexOfOffset(ref.target);
      DCHECK_NE(kUnusedIndex, index);
      if (reference_label_manager.IsIndexStored(index))
        ref.target = MarkIndex(index);
    }
  }
  label_count_[pool.value()] = label_manager.size();
}

}  // namespace zucchini
