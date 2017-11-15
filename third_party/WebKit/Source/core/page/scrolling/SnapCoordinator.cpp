// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/scrolling/SnapCoordinator.h"

#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutView.h"
#include "core/paint/PaintLayerScrollableArea.h"
#include "platform/LengthFunctions.h"
#include "platform/scroll/ScrollSnapData.h"

namespace blink {

SnapCoordinator::SnapCoordinator() : snap_container_map_() {}

SnapCoordinator::~SnapCoordinator() {}

SnapCoordinator* SnapCoordinator::Create() {
  return new SnapCoordinator();
}

// Returns the scroll container that can be affected by this snap area.
static LayoutBox* FindSnapContainer(const LayoutBox& snap_area) {
  // According to the new spec
  // https://drafts.csswg.org/css-scroll-snap/#snap-model
  // "Snap positions must only affect the nearest ancestor (on the element’s
  // containing block chain) scroll container".
  Element* viewport_defining_element =
      snap_area.GetDocument().ViewportDefiningElement();
  LayoutBox* box = snap_area.ContainingBlock();
  while (box && !box->HasOverflowClip() && !box->IsLayoutView() &&
         box->GetNode() != viewport_defining_element)
    box = box->ContainingBlock();

  // If we reach to viewportDefiningElement then we dispatch to viewport
  if (box && box->GetNode() == viewport_defining_element)
    return snap_area.GetDocument().GetLayoutView();

  return box;
}

void SnapCoordinator::SnapAreaDidChange(LayoutBox& snap_area,
                                        ScrollSnapAlign scroll_snap_align) {
  LayoutBox* old_container = snap_area.SnapContainer();
  if (scroll_snap_align.alignmentX == kSnapAlignmentNone &&
      scroll_snap_align.alignmentY == kSnapAlignmentNone) {
    snap_area.SetSnapContainer(nullptr);
    if (old_container)
      UpdateSnapContainerData(old_container);
    return;
  }

  if (LayoutBox* new_container = FindSnapContainer(snap_area)) {
    snap_area.SetSnapContainer(new_container);
    // TODO(sunyunjia): consider keep the SnapAreas in a map so it is
    // easier to update.
    // TODO(sunyunjia): Only update when the localframe doesn't need layout.
    UpdateSnapContainerData(new_container);
    if (old_container && old_container != new_container)
      UpdateSnapContainerData(old_container);
  } else {
    // TODO(majidvp): keep track of snap areas that do not have any
    // container so that we check them again when a new container is
    // added to the page.
  }
}

void SnapCoordinator::UpdateAllSnapContainerData() {
  for (const auto& entry : snap_container_map_) {
    UpdateSnapContainerData(entry.key);
  }
}

void SnapCoordinator::UpdateSnapContainerData(const LayoutBox* snap_container) {
  SnapContainerData snap_container_data(
      snap_container->Style()->GetScrollSnapType());
  snap_container_data.scrollable_x = snap_container->ScrollWidth().ToDouble() -
                                     snap_container->OffsetWidth().ToDouble();
  snap_container_data.scrollable_y = snap_container->ScrollHeight().ToDouble() -
                                     snap_container->OffsetHeight().ToDouble();
  if (SnapAreaSet* snap_areas = snap_container->SnapAreas()) {
    for (const LayoutBox* snap_area : *snap_areas) {
      snap_container_data.AddSnapAreaData(
          GetSnapAreaData(*snap_area, *snap_container));
    }
  }
  snap_container_map_.Set(snap_container, snap_container_data);
}

static LayoutUnit ClipInContainer(LayoutUnit point, LayoutUnit range) {
  if (point < 0)
    return LayoutUnit();
  return point < range ? point : range;
}

static LayoutUnit GetSnapOffset(SnapAlignment alignment,
                                SnapAxis axis,
                                LayoutRect container,
                                LayoutSize scrollable_size,
                                LayoutRect area) {
  switch (alignment) {
    case kSnapAlignmentNone:
      return LayoutUnit(-1);

    /* StartAlignment
      . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
      .                   ^                                   .
      .                   |                                   .
      .                   |snap_offset                        .
      .                   |                                   .
      .                   v                                   .
      .  \\\\\\\\\\\\\\|=================|\\\\\\\\\\\\\\\\\\  .
      .  \             |                 |                 \  .
      .  \             |    snap_area    |                 \  .
      .  \             |                 |                 \  .
      .  \             |=================|                 \  .
      .  \                                                 \  .
      .  \                                                 \  .
      .  \               snap_container                    \  .
      .  \                                                 \  .
      .  \                                                 \  .
      .  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  .
      .                                                       .
      .                scrollable_content                     .
      . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

    */
    case kSnapAlignmentStart:
      if (axis == kSnapAxisX) {
        return ClipInContainer(area.X() - container.X(),
                               scrollable_size.Width());
      } else {
        return ClipInContainer(area.Y() - container.Y(),
                               scrollable_size.Height());
      }

    /* CenterAlignment
      . . . . . . . . . . . . . . . . . . . . . . . . . . . .
      .                    ^                                .
      .                    | snap_offset                    .
      .                    v                                .
      .  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  .
      .  \                                               \  .
      .  \              snap_container                   \  .
      .  \                                               \  .
      .  \             |===============|                 \  .
      .  \             |   snap_area   |                 \  .
      .  \* * * * * * *|* * * * * * * *|* * * * * *  Center line
      .  \             |               |                 \  .
      .  \             |===============|                 \  .
      .  \                                               \  .
      .  \                                               \  .
      .  \                                               \  .
      .  \                                               \  .
      .  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  .
      .                                                     .
      .                                                     .
      .                scrollable_content                   .
      .                                                     .
      . . . . . . . . . . . . . . . . . . . . . . . . . . . .

    */
    case kSnapAlignmentCenter:
      if (axis == kSnapAxisX) {
        return ClipInContainer(area.Center().X() - container.Center().Y(),
                               scrollable_size.Width());
      } else {
        return ClipInContainer(area.Center().Y() - container.Center().Y(),
                               scrollable_size.Height());
      }

    /* EndAlignment
      . . . . . . . . . . . . . . . . . . . . . . . . . . . .
      .                    ^                                .
      .                    | snap_offset                    .
      .                    v                                .
      .  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  .
      .  \                                               \  .
      .  \                                               \  .
      .  \                                               \  .
      .  \               snap_container                  \  .
      .  \                                               \  .
      .  \                                               \  .
      .  \                                               \  .
      .  \             |===============|                 \  .
      .  \             |               |                 \  .
      .  \             |   snap_area   |                 \  .
      .  \             |               |                 \  .
      .  \\\\\\\\\\\\\\|===============|\\\\\\\\\\\\\\\\\\  .
      .                                                     .
      .                                                     .
      .               scrollable_content                    .
      .                                                     .
      . . . . . . . . . . . . . . . . . . . . . . . . . . . .

    */
    case kSnapAlignmentEnd:
      if (axis == kSnapAxisX) {
        return ClipInContainer(area.MaxX() - container.MaxY(),
                               scrollable_size.Width());
      } else {
        return ClipInContainer(area.MaxY() - container.MaxY(),
                               scrollable_size.Height());
      }
  }
}

SnapAreaData SnapCoordinator::GetSnapAreaData(const LayoutBox& snap_area,
                                              const LayoutBox& snap_container) {
  const ComputedStyle* container_style = snap_container.Style();
  const ComputedStyle* area_style = snap_area.Style();
  SnapAreaData snap_area_data;
  LayoutRect container(
      LayoutPoint(),
      LayoutSize(snap_container.OffsetWidth(), snap_container.OffsetHeight()));
  LayoutSize scrollable_size(snap_container.ScrollWidth(),
                             snap_container.ScrollHeight());
  scrollable_size -= container.Size();
  // This assumes that the snap_container is the snap_area's ancestor in layout
  // tree, as the container is found y walking up the layout tree in
  // FindSnapContainer().
  LayoutRect area(
      snap_area.OffsetPoint(ToElement(snap_container.GetNode())),
      LayoutSize(snap_area.OffsetWidth(), snap_area.OffsetHeight()));
  LayoutRectOutsets padding(
      MinimumValueForLength(container_style->ScrollPaddingTop(),
                            container.Height()),
      MinimumValueForLength(container_style->ScrollPaddingRight(),
                            container.Width()),
      MinimumValueForLength(container_style->ScrollPaddingBottom(),
                            container.Height()),
      MinimumValueForLength(container_style->ScrollPaddingLeft(),
                            container.Width()));
  LayoutRectOutsets margin(
      MinimumValueForLength(area_style->ScrollSnapMarginTop(), LayoutUnit()),
      MinimumValueForLength(area_style->ScrollSnapMarginRight(), LayoutUnit()),
      MinimumValueForLength(area_style->ScrollSnapMarginBottom(), LayoutUnit()),
      MinimumValueForLength(area_style->ScrollSnapMarginLeft(), LayoutUnit()));
  container.Contract(padding);
  area.Expand(margin);

  SnapAlignment x_start_align;
  SnapAlignment x_end_align;
  SnapAlignment y_start_align;
  SnapAlignment y_end_align;
  if (area.Width() <= container.Width()) {
    x_start_align = area_style->GetScrollSnapAlign().alignmentX;
    x_end_align = x_start_align;
  } else {
    x_start_align = kSnapAlignmentStart;
    x_end_align = kSnapAlignmentEnd;
  }
  if (area.Height() <= container.Height()) {
    y_start_align = area_style->GetScrollSnapAlign().alignmentY;
    y_end_align = y_start_align;
  } else {
    y_start_align = kSnapAlignmentStart;
    y_end_align = kSnapAlignmentEnd;
  }
  snap_area_data.snap_start_x =
      GetSnapOffset(x_start_align, kSnapAxisX, container, scrollable_size, area)
          .ToDouble();
  snap_area_data.snap_end_x =
      GetSnapOffset(x_end_align, kSnapAxisX, container, scrollable_size, area)
          .ToDouble();
  snap_area_data.snap_start_y =
      GetSnapOffset(y_start_align, kSnapAxisY, container, scrollable_size, area)
          .ToDouble();
  snap_area_data.snap_end_y =
      GetSnapOffset(y_end_align, kSnapAxisY, container, scrollable_size, area)
          .ToDouble();

  snap_area_data.visible_start_x =
      ClipInContainer(area.X() - container.MaxX(), scrollable_size.Width())
          .ToDouble();
  snap_area_data.visible_start_y =
      ClipInContainer(area.Y() - container.MaxY(), scrollable_size.Height())
          .ToDouble();
  snap_area_data.visible_end_x =
      ClipInContainer(area.MaxX() - container.X(), scrollable_size.Width())
          .ToDouble();
  snap_area_data.visible_end_y =
      ClipInContainer(area.MaxY() - container.Y(), scrollable_size.Height())
          .ToDouble();
  snap_area_data.must_snap =
      (area_style->ScrollSnapStop() == EScrollSnapStop::kAlways);

  return snap_area_data;
}

void SnapCoordinator::PrintSnapData(const LayoutBox* snap_container) {
  auto iter = snap_container_map_.find(snap_container);
  if (iter != snap_container_map_.end()) {
    SnapContainerData data = iter->value;
    ScrollSnapType type = data.scroll_snap_type;
    for (SnapAreaData snap_area_data : data.snap_area_list) {
      LOG(ERROR) << snap_area_data.snap_start_x << ", "
                 << snap_area_data.snap_start_y;
    }
  }
}

static ScrollSnapType GetSnappingType(SnapAxis axis, ScrollSnapType type) {
  if (axis == kSnapAxisBoth || axis == type.axis)
    return type;
  if (type.axis == kSnapAxisBoth) {
    type.axis = axis;
    return type;
  }
  type.is_none = true;
  return type;
}

// Returns the closest point to x on the interval [x1, x2].
static double ClosestPointOnLine(double x, double x1, double x2) {
  if (x2 < x)
    return x2;
  if (x < x1)
    return x1;
  return x;
}

ScrollOffset SnapCoordinator::FindSnapOffsetOnX(ScrollOffset current_offset,
                                                const SnapContainerData& data) {
  double smallest_distance = data.scrollable_x + 1;
  double snap_offset = current_offset.Width();
  for (SnapAreaData snap_area_data : data.snap_area_list) {
    // If it is a proximity snap, make sure the snapping element is currently
    // visible. If it is a mandatory snap, make sure the snapping element will
    // be eventually visible.
    if (current_offset.Height() >= snap_area_data.visible_start_y &&
        current_offset.Height() <= snap_area_data.visible_end_y &&
        ((current_offset.Width() >= snap_area_data.visible_start_x &&
          current_offset.Width() <= snap_area_data.visible_end_x) ||
         data.scroll_snap_type.strictness == SnapStrictness::kMandatory)) {
      double closest_point = ClosestPointOnLine(current_offset.Width(),
                                                snap_area_data.snap_start_x,
                                                snap_area_data.snap_end_x);
      double distance = std::abs(current_offset.Width() - closest_point);
      if (distance < smallest_distance) {
        smallest_distance = distance;
        snap_offset = closest_point;
      }
    }
  }
  return ScrollOffset(snap_offset, current_offset.Height());
}

ScrollOffset SnapCoordinator::FindSnapOffsetOnY(ScrollOffset current_offset,
                                                const SnapContainerData& data) {
  double smallest_distance = data.scrollable_y + 1;
  double snap_offset = current_offset.Height();
  for (SnapAreaData snap_area_data : data.snap_area_list) {
    // If it is a proximity snap, make sure the snapping element is currently
    // visible. If it is a mandatory snap, make sure the snapping element will
    // be eventually visible.
    if (current_offset.Width() >= snap_area_data.visible_start_x &&
        current_offset.Width() <= snap_area_data.visible_end_x &&
        ((current_offset.Height() >= snap_area_data.visible_start_y &&
          current_offset.Height() <= snap_area_data.visible_end_y) ||
         data.scroll_snap_type.strictness == SnapStrictness::kMandatory)) {
      double closest_point = ClosestPointOnLine(current_offset.Height(),
                                                snap_area_data.snap_start_y,
                                                snap_area_data.snap_end_y);
      double distance = std::abs(current_offset.Height() - closest_point);
      if (distance < smallest_distance) {
        smallest_distance = distance;
        snap_offset = closest_point;
      }
    }
  }
  return ScrollOffset(current_offset.Width(), snap_offset);
}

ScrollOffset SnapCoordinator::FindSnapOffsetOnBoth(
    ScrollOffset current_offset,
    const SnapContainerData& data) {
  double smallest_distance =
      std::hypot(data.scrollable_x, data.scrollable_y) + 1;
  ScrollOffset snap_offset = current_offset;
  for (SnapAreaData snap_area_data : data.snap_area_list) {
    // If it is a proximity snap, make sure the snapping element is currently
    // visible.
    if ((current_offset.Width() >= snap_area_data.visible_start_x &&
         current_offset.Width() <= snap_area_data.visible_end_x &&
         current_offset.Height() >= snap_area_data.visible_start_y &&
         current_offset.Height() <= snap_area_data.visible_end_y) ||
        data.scroll_snap_type.strictness == SnapStrictness::kMandatory) {
      ScrollOffset closest_point =
          ScrollOffset(ClosestPointOnLine(current_offset.Width(),
                                          snap_area_data.snap_start_x,
                                          snap_area_data.snap_end_x),
                       ClosestPointOnLine(current_offset.Height(),
                                          snap_area_data.snap_start_y,
                                          snap_area_data.snap_end_y));
      double distance =
          std::hypot(current_offset.Width() - closest_point.Width(),
                     current_offset.Height() - closest_point.Height());
      if (distance < smallest_distance) {
        smallest_distance = distance;
        snap_offset = closest_point;
      }
    }
  }
  return snap_offset;
}

ScrollOffset SnapCoordinator::GetSnapPosition(const LayoutBox& snap_container,
                                              SnapAxis axis) {
  ScrollOffset current_scroll_offset =
      snap_container.GetScrollableArea()->GetScrollOffset();
  auto iter = snap_container_map_.find(&snap_container);
  if (iter != snap_container_map_.end()) {
    SnapContainerData data = iter->value;
    ScrollSnapType type = GetSnappingType(axis, data.scroll_snap_type);
    if (type.is_none)
      return ScrollOffset();
    if (data.snap_area_list.size()) {
      if (type.axis == kSnapAxisX)
        return FindSnapOffsetOnX(current_scroll_offset, data);
      if (type.axis == kSnapAxisY)
        return FindSnapOffsetOnY(current_scroll_offset, data);
      if (type.axis == kSnapAxisBoth)
        return FindSnapOffsetOnBoth(current_scroll_offset, data);
    }
  }
  return ScrollOffset();
}

void SnapCoordinator::SnapContainerDidChange(LayoutBox& snap_container,
                                             ScrollSnapType scroll_snap_type) {
  if (scroll_snap_type.is_none) {
    snap_container_map_.erase(&snap_container);
    snap_container.ClearSnapAreas();
  } else {
    if (scroll_snap_type.axis == kSnapAxisInline) {
      if (snap_container.Style()->IsHorizontalWritingMode())
        scroll_snap_type.axis = kSnapAxisX;
      else
        scroll_snap_type.axis = kSnapAxisY;
    }
    if (scroll_snap_type.axis == kSnapAxisBlock) {
      if (snap_container.Style()->IsHorizontalWritingMode())
        scroll_snap_type.axis = kSnapAxisY;
      else
        scroll_snap_type.axis = kSnapAxisX;
    }
    // TODO(sunyunjia): Only update when the localframe doesn't need layout.
    UpdateSnapContainerData(&snap_container);
  }

  // TODO(majidvp): Add logic to correctly handle orphaned snap areas here.
  // 1. Removing container: find a new snap container for its orphan snap
  // areas (most likely nearest ancestor of current container) otherwise add
  // them to an orphan list.
  // 2. Adding container: may take over snap areas from nearest ancestor snap
  // container or from existing areas in orphan pool.
}

SnapContainerData SnapCoordinator::GetSnapContainerData(
    LayoutBox* snap_container) {
  auto iter = snap_container_map_.find(snap_container);
  if (iter != snap_container_map_.end()) {
    return iter->value;
  }
  return SnapContainerData();
}

#ifndef NDEBUG

void SnapCoordinator::ShowSnapAreaMap() {
  for (auto& container : snap_containers_.Keys())
    ShowSnapAreasFor(container);
}

void SnapCoordinator::ShowSnapAreasFor(const LayoutBox* container) {
  LOG(INFO) << *container->GetNode();
  if (SnapAreaSet* snap_areas = container->SnapAreas()) {
    for (auto& snap_area : *snap_areas) {
      LOG(INFO) << "    " << *snap_area->GetNode();
    }
  }
}

#endif

}  // namespace blink
