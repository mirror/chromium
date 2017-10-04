// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/encoded_view.h"

#include <algorithm>

#include "base/logging.h"

namespace zucchini {

EncodedView::EncodedView(const ImageIndex& image_index)
    : image_index_(image_index), pools_(image_index.PoolCount()) {}
EncodedView::~EncodedView() = default;

EncodedView::value_type EncodedView::Projection(offset_t location) const {
  DCHECK_LT(location, image_index_.size());

  // Find out what lies at |location|.
  TypeTag type = image_index_.LookupType(location);

  // |location| points into raw data.
  if (type == kNoTypeTag) {
    // The projection is the identity function on raw content.
    return image_index_.GetRawValue(location);
  }

  // |location| points into a Reference.
  IndirectReference ref = image_index_.at(type).at(location);
  DCHECK_GE(location, ref.location);
  DCHECK_LT(location, ref.location + image_index_.at(type).width());

  // |location| is not the first byte of the reference.
  if (location != ref.location) {
    // Trailing bytes of a reference are all projected to the same value.
    return kReferencePaddingProjection;
  }

  PoolTag pool_tag = image_index_.at(type).pool_tag();

  // Targets with an associated Label will use its Label index in projection.
  // Otherwise, LabelBound() is used for all targets with no Label.
  DCHECK_EQ(image_index_.at(pool_tag).size(),
            pools_[pool_tag.value()].labels.size());
  value_type label =
      pools_[image_index_.at(type).pool_tag().value()].labels[ref.target_key];

  // Projection is done on (|target|, |type|), shifted by a constant value to
  // avoid collisions with raw content.
  value_type projection = label;
  projection *= image_index_.TypeCount();
  projection += type.value();
  return projection + kBaseReferenceProjection;
}

size_t EncodedView::Cardinality() const {
  size_t max_width = 0;
  for (uint8_t pool = 0; pool < image_index_.PoolCount(); ++pool)
    max_width = std::max(pools_[pool].bound, max_width);
  return max_width * image_index_.TypeCount() + kBaseReferenceProjection;
}

EncodedView::PoolInfo::PoolInfo() = default;
EncodedView::PoolInfo::PoolInfo(PoolInfo&&) = default;
EncodedView::PoolInfo::~PoolInfo() = default;

}  // namespace zucchini
