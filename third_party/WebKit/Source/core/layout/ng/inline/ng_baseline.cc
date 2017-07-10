// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_baseline.h"

#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/ng/inline/ng_line_box_fragment.h"
#include "core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "core/layout/ng/ng_block_node.h"
#include "core/layout/ng/ng_box_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

bool NGBaseline::ShouldPropagateBaselines(const NGLayoutInputNode node) {
  if (node.IsInline())
    return true;

  return ShouldPropagateBaselines(ToLayoutBox(node.GetLayoutObject()));
}

bool NGBaseline::ShouldPropagateBaselines(const LayoutBox* layout_obj) {
  if (!layout_obj->IsLayoutBlock() ||
      layout_obj->IsFloatingOrOutOfFlowPositioned() ||
      layout_obj->IsWritingModeRoot())
    return false;

  const LayoutBlock* layout_block = ToLayoutBlock(layout_obj);
  if (layout_block->UseLogicalBottomMarginEdgeForInlineBlockBaseline())
    return false;

  return true;
}

}  // namespace blink
