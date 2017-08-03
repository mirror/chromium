// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/encoded_view.h"

#include <algorithm>
#include <cassert>

namespace zucchini {

EncodedView::value_type EncodedView::Rank(const ImageIndex& image_index,
                                          offset_t location) {
  DCHECK_LT(location, image_index.size());

  // Find out what lies at |location|.
  TypeTag type = image_index.GetType(location);

  // |location| points into raw data.
  if (type == kNoTypeTag) {
    return image_index.GetRawValue(location);
  }

  // |location| points into a Reference.
  Reference ref = image_index.FindReference(type, location);
  DCHECK(location >= ref.location);
  DCHECK(location < ref.location + image_index.GetTraits(type).width);
  // |location| is not the boss.
  if (location != ref.location) {
    // Assign dummy value for non-token reference bytes.
    return kReferencePaddingRank;
  }

  size_t target = IsMarked(ref.target)
                      ? UnmarkIndex(ref.target)
                      : image_index.LabelBound(image_index.GetPoolTag(type));

  // We encode target and type within rank.
  size_t rank = target;
  rank *= image_index.TypeCount();
  rank += type.value();
  return rank + kBaseRefRank;
}

size_t EncodedView::Cardinality() const {
  size_t max_width = 0;
  for (uint8_t pool = 0; pool < image_index_->PoolCount(); ++pool) {
    // label_count + 1 for the extra case where a Reference is Unmarked.
    max_width =
        std::max(image_index_->LabelBound(PoolTag(pool)) + 1, max_width);
  }
  return max_width * image_index_->TypeCount() + kBaseRefRank;
}

}  // namespace zucchini
