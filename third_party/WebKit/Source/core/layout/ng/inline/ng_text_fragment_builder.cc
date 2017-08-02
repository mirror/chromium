// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_text_fragment_builder.h"

#include "core/layout/ng/inline/ng_inline_node.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_fragment.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_unpositioned_float.h"
#include "platform/heap/Handle.h"

namespace blink {

namespace {

NGLineOrientation ToLineOrientation(NGWritingMode writing_mode) {
  switch (writing_mode) {
    case NGWritingMode::kHorizontalTopBottom:
      return NGLineOrientation::kHorizontal;
    case NGWritingMode::kVerticalRightLeft:
    case NGWritingMode::kVerticalLeftRight:
    case NGWritingMode::kSidewaysRightLeft:
      return NGLineOrientation::kClockWiseVertical;
    case NGWritingMode::kSidewaysLeftRight:
      return NGLineOrientation::kCounterClockWiseVertical;
  }
  NOTREACHED();
  return NGLineOrientation::kHorizontal;
}

}  // namespace

NGTextFragmentBuilder::NGTextFragmentBuilder(NGInlineNode node,
                                             bool use_first_line_style)
    : NGBaseFragmentBuilder(
          FromPlatformWritingMode(node_.Style().GetWritingMode()),
          TextDirection::kLtr,
          use_first_line_style),
      node_(node) {}

NGTextFragmentBuilder& NGTextFragmentBuilder::SetSize(
    const NGLogicalSize& size) {
  size_ = size;
  return *this;
}

NGTextFragmentBuilder& NGTextFragmentBuilder::SetShapeResult(
    RefPtr<const ShapeResult> shape_result) {
  shape_result_ = shape_result;
  return *this;
}

RefPtr<NGPhysicalTextFragment> NGTextFragmentBuilder::ToTextFragment(
    unsigned index,
    unsigned start_offset,
    unsigned end_offset) {
  return AdoptRef(new NGPhysicalTextFragment(
      node_.GetLayoutObject(), use_first_line_style_, node_.Text(), index,
      start_offset, end_offset, size_.ConvertToPhysical(writing_mode_),
      ToLineOrientation(writing_mode_), std::move(shape_result_)));
}

}  // namespace blink
