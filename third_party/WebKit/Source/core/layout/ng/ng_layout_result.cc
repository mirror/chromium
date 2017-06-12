// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_layout_result.h"

namespace blink {

NGLayoutResult::NGLayoutResult(
    PassRefPtr<NGPhysicalFragment> physical_fragment,
    const Vector<NGBlockNode>& out_of_flow_descendants,
    const Vector<NGStaticPosition> out_of_flow_positions,
    Vector<RefPtr<NGUnpositionedFloat>>& unpositioned_floats)
    : physical_fragment_(std::move(physical_fragment)),
      out_of_flow_descendants_(out_of_flow_descendants),
      out_of_flow_positions_(out_of_flow_positions) {
  unpositioned_floats_.swap(unpositioned_floats);
}

RefPtr<NGLayoutResult> NGLayoutResult::CloneWithoutOffset() const {
  Vector<RefPtr<NGUnpositionedFloat>> unpositioned_floats(unpositioned_floats_);
  return AdoptRef(new NGLayoutResult(
      physical_fragment_->CloneWithoutOffset(), out_of_flow_descendants_,
      out_of_flow_positions_, unpositioned_floats));
}

}  // namespace blink
