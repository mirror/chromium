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
    : element_id_(element_id),
      orientation_(orientation),
      time_range_(time_range) {}

std::unique_ptr<ScrollTimeline> ScrollTimeline::CreateImplInstance() const {
  return std::make_unique<ScrollTimeline>(element_id_, orientation_,
                                          time_range_);
}

// TODO(smcgruer): The code here is very close to that in Blink. Can we merge?
double ScrollTimeline::CurrentTime(const ScrollTree& scroll_tree) const {
  // TODO(smcgruer): Handle this being null.
  DCHECK(scroll_tree.FindNodeFromElementId(element_id_));

  // TODO(smcgruer): Check if offset is absolute or not (the dchecks will fail
  // if it isnt and we'll need to do something else).
  gfx::ScrollOffset offset = scroll_tree.current_scroll_offset(element_id_);
  DCHECK_GE(offset.x(), 0);
  DCHECK_GE(offset.y(), 0);

  gfx::ScrollOffset scroll_dimensions = scroll_tree.MaxScrollOffset(
      scroll_tree.FindNodeFromElementId(element_id_)->id);

  double current_offset = (orientation_ == Vertical) ? offset.y() : offset.x();
  double max_offset = (orientation_ == Vertical) ? scroll_dimensions.y()
                                                 : scroll_dimensions.x();

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

  return (std::abs(current_offset) / max_offset) * time_range_;
}

}  // namespace cc
