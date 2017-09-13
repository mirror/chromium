// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/layout_ng_data.h"

#include "core/layout/LayoutBox.h"
#include "core/layout/ng/inline/ng_inline_node_data.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_layout_result.h"

namespace blink {

NGInlineNodeData* LayoutNGData::GetNGInlineNodeData() const {
  DCHECK(ng_inline_node_data_);
  return ng_inline_node_data_.get();
}

void LayoutNGData::ResetNGInlineNodeData() {
  ng_inline_node_data_ = WTF::MakeUnique<NGInlineNodeData>();
}

RefPtr<NGLayoutResult> LayoutNGData::CachedLayoutResult(
    LayoutBox* box,
    const NGConstraintSpace& constraint_space,
    NGBreakToken* break_token) const {
  if (!cached_result_ || break_token || box->NeedsLayout())
    return nullptr;
  if (constraint_space != *cached_constraint_space_)
    return nullptr;
  return cached_result_->CloneWithoutOffset();
}

void LayoutNGData::SetCachedLayoutResult(
    const NGConstraintSpace& constraint_space,
    NGBreakToken* break_token,
    RefPtr<NGLayoutResult> layout_result) {
  if (!RuntimeEnabledFeatures::LayoutNGFragmentCachingEnabled())
    return;
  if (break_token || constraint_space.UnpositionedFloats().size() ||
      layout_result->UnpositionedFloats().size() ||
      layout_result->Status() != NGLayoutResult::kSuccess) {
    // We can't cache these yet
    return;
  }

  cached_constraint_space_ = &constraint_space;
  cached_result_ = layout_result;
}

}  // namespace blink
