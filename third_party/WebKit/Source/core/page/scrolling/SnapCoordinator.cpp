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
  LayoutBox* new_container = nullptr;
  if (scroll_snap_align.alignmentX == kSnapAlignmentNone &&
      scroll_snap_align.alignmentY == kSnapAlignmentNone) {
    snap_area.SetSnapContainer(nullptr);
  } else if (LayoutBox* snap_container = FindSnapContainer(snap_area)) {
    snap_area.SetSnapContainer(snap_container);
    new_container = snap_container;
  } else {
    // TODO(majidvp): keep track of snap areas that do not have any
    // container so that we check them again when a new container is
    // added to the page.
  }
  // TODO(sunyunjia): consider keep the SnapAreas in a map so it is
  // easier to update.
  // TODO(sunyunjia): Only update when the localframe doesn't need layout.
  if (old_container)
    UpdateSnapContainerData(old_container);
  if (new_container)
    UpdateSnapContainerData(new_container);
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

static double ClipInContainer(double point, double range) {
  if (point < 0)
    return 0;
  return point < range ? point : range;
}

static double GetSnapOffset(SnapAlignment alignment,
                            double container_size,
                            double scrollable_range,
                            double area_size,
                            double area_offset,
                            double padding_start,
                            double padding_end,
                            double margin_start,
                            double margin_end) {
  switch (alignment) {
    case kSnapAlignmentNone:
      return -1;
    case kSnapAlignmentStart:
      return ClipInContainer(area_offset - padding_start - margin_start,
                             scrollable_range);
    case kSnapAlignmentCenter:
      // TODO(sunyunjia): Should we consider margin when doing ceter alignment?
      return ClipInContainer(
          area_offset + area_size / 2 -
              (container_size + padding_start - padding_end) / 2,
          scrollable_range);
    case kSnapAlignmentEnd:
      return ClipInContainer(
          area_offset - container_size + padding_end + margin_end + area_size,
          scrollable_range);
  }
}

SnapAreaData SnapCoordinator::GetSnapAreaData(const LayoutBox& snap_area,
                                              const LayoutBox& snap_container) {
  const ComputedStyle* container_style = snap_container.Style();
  const ComputedStyle* area_style = snap_area.Style();
  SnapAreaData snap_area_data;
  double container_width = snap_container.OffsetWidth().ToDouble();
  double container_height = snap_container.OffsetHeight().ToDouble();
  double scrollable_x =
      snap_container.ScrollWidth().ToDouble() - container_width;
  double scrollable_y =
      snap_container.ScrollHeight().ToDouble() - container_height;
  double area_width = snap_area.OffsetWidth().ToDouble();
  double area_height = snap_area.OffsetHeight().ToDouble();
  double area_offset_x =
      snap_area.OffsetLeft(ToElement(snap_container.GetNode())).ToDouble();
  double area_offset_y =
      snap_area.OffsetTop(ToElement(snap_container.GetNode())).ToDouble();

  Length pad_left = container_style->ScrollPaddingLeft();
  Length pad_right = container_style->ScrollPaddingRight();
  Length pad_top = container_style->ScrollPaddingTop();
  Length pad_bottome = container_style->ScrollPaddingBottom();
  double padding_left = pad_left.IsPercent()
                            ? pad_left.GetFloatValue() * container_width / 100
                            : pad_left.GetFloatValue();
  double padding_right = pad_left.IsPercent()
                             ? pad_left.GetFloatValue() * container_width / 100
                             : pad_left.GetFloatValue();
  double padding_top = pad_left.IsPercent()
                           ? pad_left.GetFloatValue() * container_width / 100
                           : pad_left.GetFloatValue();
  double padding_bottom = pad_left.IsPercent()
                              ? pad_left.GetFloatValue() * container_width / 100
                              : pad_left.GetFloatValue();

  double margin_left = area_style->ScrollSnapMarginLeft().GetFloatValue();
  double margin_right = area_style->ScrollSnapMarginRight().GetFloatValue();
  double margin_top = area_style->ScrollSnapMarginTop().GetFloatValue();
  double margin_bottom = area_style->ScrollSnapMarginBottom().GetFloatValue();

  if (area_width <= container_width) {
    snap_area_data.snap_start_x =
        GetSnapOffset(area_style->GetScrollSnapAlign().alignmentX,
                      container_width, scrollable_x, area_width, area_offset_x,
                      padding_left, padding_right, margin_left, margin_right);
    snap_area_data.snap_end_x = snap_area_data.snap_start_x;
  } else {
    snap_area_data.snap_start_x = GetSnapOffset(
        kSnapAlignmentStart, container_width, scrollable_x, area_width,
        area_offset_x, padding_left, padding_right, margin_left, margin_right);
    snap_area_data.snap_end_x = GetSnapOffset(
        kSnapAlignmentEnd, container_width, scrollable_x, area_width,
        area_offset_x, padding_left, padding_right, margin_left, margin_right);
  }
  if (area_height <= container_height) {
    snap_area_data.snap_start_y = GetSnapOffset(
        area_style->GetScrollSnapAlign().alignmentY, container_height,
        scrollable_y, area_height, area_offset_y, padding_top, padding_bottom,
        margin_top, margin_bottom);
    snap_area_data.snap_end_y = snap_area_data.snap_start_y;
  } else {
    snap_area_data.snap_start_y = GetSnapOffset(
        kSnapAlignmentStart, container_height, scrollable_y, area_height,
        area_offset_y, padding_top, padding_bottom, margin_top, margin_bottom);
    snap_area_data.snap_end_y = GetSnapOffset(
        kSnapAlignmentEnd, container_height, scrollable_y, area_height,
        area_offset_y, padding_top, padding_bottom, margin_top, margin_bottom);
  }
  snap_area_data.visible_start_x = ClipInContainer(
      area_offset_x - container_width + padding_right, scrollable_x);
  snap_area_data.visible_start_y = ClipInContainer(
      area_offset_y - container_height + padding_bottom, scrollable_y);
  snap_area_data.visible_end_x =
      ClipInContainer(area_offset_x + area_width - padding_left, scrollable_x);
  snap_area_data.visible_end_y =
      ClipInContainer(area_offset_y + area_height - padding_top, scrollable_y);
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
