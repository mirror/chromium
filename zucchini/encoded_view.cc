// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/encoded_view.h"

#include <algorithm>

namespace zucchini {

EncodedView::EncodedView(Region image,
                         const ReferenceHolder& refs,
                         size_t label_count)
    : references_(&refs),
      image_(image),
      label_count_(references_->TypeCount(), 1) {
  // |types_| is an array as big as |image| and filled with the type of
  // References at the same index, or |kNoRefType| if not a Reference.
  types_.resize(image_.size(), kNoRefType);
  for (auto ref : references_->Get(SortedByType)) {
    for (offset_t i = 0; i < references_->Width(ref.type); ++i) {
      types_[ref.location + i] = ref.type;
    }
  }
}

EncodedView::~EncodedView() = default;

void EncodedView::SetLabelCount(uint8_t pool, size_t label_count) {
  label_count_[pool] = label_count;
}

size_t EncodedView::Rank(offset_t idx) const {
  // Find out what lies at |idx| using |types_|.
  uint8_t type = types_[idx];

  // |idx| points into raw data.
  if (type == kNoRefType) {
    return image_[idx];
  }

  // |idx| points into a Reference.
  Reference ref = references_->Find(type, idx);
  assert(idx >= ref.location);
  // |idx| is not the boss.
  if (idx != ref.location) {
    // Assign dummy value for non-token reference bytes.
    return kReferencePaddingRank;
  }

  offset_t target = IsMarked(ref.target)
                        ? UnmarkIndex(ref.target)
                        : offset_t(label_count_[references_->Pool(type)]);

  // We encode target and type within rank.
  size_t rank = target;
  rank *= references_->TypeCount();
  rank += type;
  return rank + kBaseRefRank;
}

size_t EncodedView::Cardinality() const {
  size_t max_width = 0;
  for (uint8_t type = 0; type < references_->TypeCount(); ++type) {
    // label_count + 1 for the extra case where a Reference is Unmarked.
    max_width = std::max(label_count_[references_->Pool(type)] + 1, max_width);
  }
  return max_width * references_->TypeCount() + kBaseRefRank;
}

int EncodedView::Distance(size_t a, size_t b) const {
  if (a >= kBaseRefRank && b >= kBaseRefRank) {
    // Both |a| and |b| are References.
    a -= kBaseRefRank;
    b -= kBaseRefRank;

    uint8_t type = uint8_t(a % references_->TypeCount());
    // Both |a| and |b| must be of the same type.
    if (b % references_->TypeCount() != type)
      return kMismatchFatal;
    a /= references_->TypeCount();
    b /= references_->TypeCount();

    // We are left with the target of |a| and |b|.
    if (a != b)
      return kMismatchReference;
    else
      return 0;

  } else if (a < kBaseRefRank && b < kBaseRefRank) {
    // Both |a| and |b| are raw data.
    if (a != b)
      return kMismatchRaw;
    else
      return 0;

  } else {
    // We can not match a Reference against raw data.
    return kMismatchFatal;
  }
}

bool EncodedView::IsToken(offset_t idx) const {
  // Find out what lies at |idx| using |types_|.
  uint8_t type = types_[idx];

  // |idx| points into raw data.
  if (type == kNoRefType) {
    return true;
  }

  // |idx| points into a Reference.
  Reference ref = references_->Find(type, idx);
  assert(idx >= ref.location);
  return idx == ref.location;  // Only the first byte of a reference is a token.
}

}  // namespace zucchini
