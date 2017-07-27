// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_index.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include "chrome/installer/zucchini/disassembler.h"

namespace zucchini {

ImageIndex::ImageIndex(ConstBufferView image)
    : raw_image_(image), type_tags_(image.size(), kNoTypeTag) {}

ImageIndex::~ImageIndex() = default;

bool ImageIndex::InsertReferences(const ReferenceTypeTraits& traits,
                                  ReferenceReader&& ref_reader) {
  if (traits.type_tag.value() >= references_.size()) {
    // O(n^2) worst case, but these vectors are small so this is okay.
    references_.resize(traits.type_tag.value() + 1);
    traits_.resize(traits.type_tag.value() + 1);
  }

  DCHECK_LT(traits.type_tag.value(), traits_.size());
  traits_[traits.type_tag.value()] = traits;

  pool_count_ = std::max(pool_count_, uint8_t(traits.pool_tag.value() + 1));

  for (auto ref = ref_reader.GetNext(); ref; ref = ref_reader.GetNext()) {
    DCHECK_LE(ref->location + traits.width, size());

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

base::Optional<Reference> ImageIndex::FindReference(TypeTag type,
                                                    offset_t upper) const {
  auto pos = std::upper_bound(
      references_[type.value()].begin(), references_[type.value()].end(), upper,
      [](offset_t a, const Reference& ref) { return a < ref.location; });

  if (pos == references_[type.value()].begin())
    return base::nullopt;
  --pos;
  if (upper < pos->location + GetTraits(type).width)
    return *pos;
  return base::nullopt;
}

bool ImageIndex::IsToken(offset_t location) const {
  TypeTag type = GetType(location);

  // |location| points into raw data.
  if (type == kNoTypeTag) {
    return true;
  }

  // |location| points into a Reference.
  auto reference = FindReference(type, location);
  DCHECK(reference.has_value());
  // Only the first byte of a reference is a token.
  return location == reference->location;
}

}  // namespace zucchini
