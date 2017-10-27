// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/scroll_timeline.h"

#include "cc/trees/property_tree.h"
#include "cc/trees/scroll_node.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/geometry/size.h"

namespace cc {

ScrollTimeline::ScrollTimeline(ElementId element_id,
                               ScrollDirection orientation,
                               double time_range)
    : element_id(element_id),
      orientation(orientation),
      time_range(time_range) {}

// TODO(smcgruer): The code here is fundamentally equivalent to that in Blink.
// They should be merged.
double ScrollTimeline::CurrentTime(ScrollTree* scroll_tree) const {
  // TODO(smcgruer): We should be able to take a const-ref here, but
  // ScrollTree::FindNodeFromElementId is non-const.
  DCHECK(scroll_tree);

  // TODO(smcgruer): Handle this being null.
  DCHECK(scroll_tree->FindNodeFromElementId(element_id));

  // TODO(smcgruer): Check if offset is absolute or not (the dchecks will fail
  // if it isnt and we'll need to do something else).
  gfx::ScrollOffset offset = scroll_tree->current_scroll_offset(element_id);
  DCHECK_GE(offset.x(), 0);
  DCHECK_GE(offset.y(), 0);

  gfx::ScrollOffset scroll_dimensions = scroll_tree->MaxScrollOffset(
      scroll_tree->FindNodeFromElementId(element_id)->id);

  double current_offset;
  double max_offset;
  // TODO(smcgruer): Handle writing mode
  bool is_horizontal = true;  // layout_box->IsHorizontalWritingMode();
  if (orientation == Block) {
    current_offset = is_horizontal ? offset.y() : offset.x();
    max_offset = is_horizontal ? scroll_dimensions.y() : scroll_dimensions.x();
  } else {
    DCHECK(orientation == Inline);
    current_offset = is_horizontal ? offset.x() : offset.y();
    max_offset = is_horizontal ? scroll_dimensions.x() : scroll_dimensions.y();
  }

  // 3. If current scroll offset is less than startScrollOffset, return an
  // unresolved time value if fill is none or forwards, or 0 otherwise.
  // TODO(smcgruer): Implement |startScrollOffset| and |fill|.

  // 4. If current scroll offset is greater than or equal to endScrollOffset,
  // return an unresolved time value if fill is none or backwards, or the
  // effective time range otherwise.
  // TODO(smcgruer): Implement |endScrollOffset| and |fill|.

  // 5. Return the result of evaluating the following expression:
  //   ((current scroll offset - startScrollOffset) /
  //      (endScrollOffset - startScrollOffset)) * effective time range
  return (std::abs(current_offset) / max_offset) * time_range;
}

}  // namespace cc
