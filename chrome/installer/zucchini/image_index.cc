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
    // Build pool-to-type mapping.
    DCHECK_NE(kNoPoolTag, group.pool_tag());
    target_pools_[group.pool_tag()].AddType(group.type_tag());

    target_pools_.at(group.pool_tag())
        .InsertTargets(std::move(*group.GetReader(disasm)));
  }
  for (const auto& group : ref_groups) {
    // Find and store all references for current type, returns false on finding
    // any overlap, to signal error.
    if (!InsertReferences(group.traits(),
                          std::move(*group.GetReader(disasm)))) {
      return false;
    }
  }
  return true;
}

bool ImageIndex::IsToken(offset_t location) const {
  TypeTag type = LookupType(location);

  // |location| points into raw data.
  if (type == kNoTypeTag)
    return true;

  // |location| points into a Reference.
  IndirectReference reference = at(type).at(location);
  // Only the first byte of a reference is a token.
  return location == reference.location;
}

bool ImageIndex::InsertReferences(const ReferenceTypeTraits& traits,
                                  ReferenceReader&& ref_reader) {
  // Store ReferenceSet for current type (of |group|).
  DCHECK_NE(kNoTypeTag, traits.type_tag);
  auto result = reference_sets_.emplace(traits.type_tag, traits);
  DCHECK(result.second);

  result.first->second.InsertReferences(std::move(ref_reader),
                                        target_pools_.at(traits.pool_tag));
  for (auto ref : reference_sets_.at(traits.type_tag)) {
    DCHECK_LE(ref.location + traits.width, size());

    // Check for overlap with existing reference. If found, then invalidate.
    if (std::any_of(type_tags_.begin() + ref.location,
                    type_tags_.begin() + ref.location + traits.width,
                    [](TypeTag type) { return type != kNoTypeTag; }))
      return false;
    std::fill(type_tags_.begin() + ref.location,
              type_tags_.begin() + ref.location + traits.width,
              traits.type_tag);
  }
  return true;
}

}  // namespace zucchini
