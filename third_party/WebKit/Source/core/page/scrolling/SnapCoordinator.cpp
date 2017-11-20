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
  if (scroll_snap_align.alignmentX == SnapAlignment::kNone &&
      scroll_snap_align.alignmentY == SnapAlignment::kNone) {
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
  snap_container_data.max_offset.set_x(
      (snap_container->ScrollWidth() - snap_container->OffsetWidth())
          .ToFloat());
  snap_container_data.max_offset.set_y(
      (snap_container->ScrollHeight() - snap_container->OffsetHeight())
          .ToFloat());
  if (SnapAreaSet* snap_areas = snap_container->SnapAreas()) {
    for (const LayoutBox* snap_area : *snap_areas) {
      snap_container_data.AddSnapAreaData(
          CalculateSnapAreaData(*snap_area, *snap_container));
    }
  }
  snap_container_map_.Set(snap_container, snap_container_data);
}

static LayoutUnit ClipInContainer(LayoutUnit value, LayoutUnit max) {
  value = value.ClampNegativeToZero();
  return value < max ? value : max;
}

// Returns scroll offset at which the snap area and snap containers meet the
// requested snapping alignment on the given axis.
// If the scroll offset required for the alignment is outside the valid range
// then it will be clamped.
// alignment - The scroll-snap-align specified on the snap area.
//    https://www.w3.org/TR/css-scroll-snap-1/#scroll-snap-align
// axis - The axis for which we consider alignment on. Should be either X or Y
// container - The snap_container rect relative to the container_element's
//    boundary. Note that this rect is represented by the dotted box below,
//    which is contracted by the scroll-padding from the element's original
//    boundary.
// scrollable_size - The maximal scrollable offset of the container. The
//    calculated snap_offset can not be larger than this value.
// area - The snap area rect relative to the snap container's boundary. Note
//    that this rect is represented by the dotted box below, which is expanded
//    by the scroll-snap-margin from the element's original boundary.
static LayoutUnit CalculateSnapOffset(SnapAlignment alignment,
                                      SnapAxis axis,
                                      LayoutRect container,
                                      LayoutSize scrollable_size,
                                      LayoutRect area) {
  DCHECK(axis == SnapAxis::kX || axis == SnapAxis::kY);
  switch (alignment) {
    /* Start alignment aligns the area's start edge with container's start edge.
       https://www.w3.org/TR/css-scroll-snap-1/#valdef-scroll-snap-align-start
      + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
      +                   ^                                     +
      +                   |                                     +
      +                   |snap_offset                          +
      +                   |                                     +
      +                   v                                     +
      +  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  +
      +  \                 scroll-padding                    \  +
      +  \   . . . . . . . . . . . . . . . . . . . . . . .   \  +
      +  \   .       .   scroll-snap-margin  .           .   \  +
      +  \   .       .  |=================|  .           .   \  +
      +  \   .       .  |                 |  .           .   \  +
      +  \   .       .  |    snap_area    |  .           .   \  +
      +  \   .       .  |                 |  .           .   \  +
      +  \   .       .  |=================|  .           .   \  +
      +  \   .       .                       .           .   \  +
      +  \   .       . . . . . . . . . . . . .           .   \  +
      +  \   .                                           .   \  +
      +  \   .                                           .   \  +
      +  \   .                                           .   \  +
      +  \   . . . . . . .snap_container . . . . . . . . .   \  +
      +  \                                                   \  +
      +  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  +
      +                                                         +
      +                scrollable_content                       +
      + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +

    */
    case SnapAlignment::kStart:
      if (axis == SnapAxis::kX) {
        return ClipInContainer(area.X() - container.X(),
                               scrollable_size.Width());
      }
      return ClipInContainer(area.Y() - container.Y(),
                             scrollable_size.Height());

    /* Center alignment aligns the snap_area(with margin)'s center line with
       snap_container(without padding)'s center line.
       https://www.w3.org/TR/css-scroll-snap-1/#valdef-scroll-snap-align-center
      + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
      +                    ^                                    +
      +                    | snap_offset                        +
      +                    v                                    +
      +  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  +
      +  \                 scroll-padding                    \  +
      +  \   . . . . . . . . . . . . . . . . . . . . . . .   \  +
      +  \   .                                           .   \  +
      +  \   .       . . . . . . . . . . . . .           .   \  +
      +  \   .       .   scroll-snap-margin  .           .   \  +
      +  \   .       .  |=================|  .           .   \  +
      +  \   .       .  |    snap_area    |  .           .   \  +
      +  \* *.* * * *.* * * * * * * * * * * *.* * * * * * * * * * Center line
      +  \   .       .  |                 |  .           .   \  +
      +  \   .       .  |=================|  .           .   \  +
      +  \   .       .                       .           .   \  +
      +  \   .       . . . . . . . . . . . . .           .   \  +
      +  \   .                                           .   \  +
      +  \   . . . . . . snap_container. . . . . . . . . .   \  +
      +  \                                                   \  +
      +  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  +
      +                                                         +
      +                                                         +
      +                scrollable_content                       +
      +                                                         +
      + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +

    */
    case SnapAlignment::kCenter:
      if (axis == SnapAxis::kX) {
        return ClipInContainer(area.Center().X() - container.Center().X(),
                               scrollable_size.Width());
      }
      return ClipInContainer(area.Center().Y() - container.Center().Y(),
                             scrollable_size.Height());

    /* End alignment aligns the snap_area(with margin)'s end edge with
       snap_container(without padding)'s end edge.
       https://www.w3.org/TR/css-scroll-snap-1/#valdef-scroll-snap-align-end
      + + + + + + + + + + + + + + + + + + + + + + + + + + + + . .
      +                    ^                                    +
      +                    | snap_offset                        +
      +                    v                                    +
      +  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  +
      +  \                                                   \  +
      +  \   . . . . . . . . snap_container. . . . . . . .   \  +
      +  \   .                                           .   \  +
      +  \   .                                           .   \  +
      +  \   .                                           .   \  +
      +  \   .       . . . . . . . . . . . . .           .   \  +
      +  \   .       .   scroll-snap-margin  .           .   \  +
      +  \   .       .  |=================|  .           .   \  +
      +  \   .       .  |                 |  .           .   \  +
      +  \   .       .  |    snap_area    |  .           .   \  +
      +  \   .       .  |                 |  .           .   \  +
      +  \   .       .  |=================|  .           .   \  +
      +  \   .       .                       .           .   \  +
      +  \   . . . . . . . . . . . . . . . . . . . . . . .   \  +
      +  \               scroll-padding                      \  +
      +  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\  +
      +                                                         +
      +               scrollable_content                        +
      +                                                         +
      + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +

    */
    case SnapAlignment::kEnd:
      if (axis == SnapAxis::kX) {
        return ClipInContainer(area.MaxX() - container.MaxX(),
                               scrollable_size.Width());
      }
      return ClipInContainer(area.MaxY() - container.MaxY(),
                             scrollable_size.Height());
    default:
      return LayoutUnit(SnapAreaData::kInvalidScrollOffset);
  }
}

SnapAreaData SnapCoordinator::CalculateSnapAreaData(
    const LayoutBox& snap_area,
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
  // We assume that the snap_container is the snap_area's ancestor in layout
  // tree, as the snap_container is found by walking up the layout tree in
  // FindSnapContainer(). Under this assumption,
  // snap_area.LocalToAncestorPoint(FloatPoint(0, 0), snap_container) returns
  // the snap_area's position relative to its container. And the |area| below
  // represents the snap_area rect in respect to the snap_container.
  LayoutRect area(
      LayoutPoint(
          snap_area.LocalToAncestorPoint(FloatPoint(0, 0), &snap_container)),
      LayoutSize(snap_area.OffsetWidth(), snap_area.OffsetHeight()));
  LayoutRectOutsets container_padding(
      // The percentage of scroll-padding is different from that of normal
      // padding, as scroll-padding resolves the percentage against
      // corresponding dimension of the scrollport[1], while the normal padding
      // resolves that against "width".[2,3]
      // When the Length's type is kAuto, MinimumValueForLength() returns
      // LayoutUnit(), which is 0, while ValueForLength() returns the value of
      // the second parameter. Although only numbers and percentages are allowed
      // here, we do not want padding to be the size of the container in the
      // event of kAuto.
      // [1] https://drafts.csswg.org/css-scroll-snap-1/#scroll-padding
      //     "relative to the corresponding dimension of the scroll container’s
      //      scrollport"
      // [2] https://drafts.csswg.org/css-box/#padding-props
      // [3] See for example LayoutBoxModelObject::ComputedCSSPadding where it
      //     uses |MinimumValueForLength| but against the "width".
      MinimumValueForLength(container_style->ScrollPaddingTop(),
                            container.Height()),
      MinimumValueForLength(container_style->ScrollPaddingRight(),
                            container.Width()),
      MinimumValueForLength(container_style->ScrollPaddingBottom(),
                            container.Height()),
      MinimumValueForLength(container_style->ScrollPaddingLeft(),
                            container.Width()));
  LayoutRectOutsets area_margin(
      // According to spec
      // https://www.w3.org/TR/css-scroll-snap-1/#scroll-snap-margin
      // Only fixed values are valid inputs. So we would return 0 for percentage
      // values.
      // TODO(sunyunjia): Update CSS parser to reject percentage values and
      // return LayoutUnit for scroll-snap-margin.
      MinimumValueForLength(area_style->ScrollSnapMarginTop(), LayoutUnit()),
      MinimumValueForLength(area_style->ScrollSnapMarginRight(), LayoutUnit()),
      MinimumValueForLength(area_style->ScrollSnapMarginBottom(), LayoutUnit()),
      MinimumValueForLength(area_style->ScrollSnapMarginLeft(), LayoutUnit()));
  container.Contract(container_padding);
  area.Expand(area_margin);

  ScrollSnapAlign align = area_style->GetScrollSnapAlign();
  snap_area_data.snap_offset.set_x(CalculateSnapOffset(align.alignmentX,
                                                       SnapAxis::kX, container,
                                                       scrollable_size, area)
                                       .ToFloat());
  snap_area_data.snap_offset.set_y(CalculateSnapOffset(align.alignmentY,
                                                       SnapAxis::kY, container,
                                                       scrollable_size, area)
                                       .ToFloat());

  if (align.alignmentX != SnapAlignment::kNone &&
      align.alignmentY != SnapAlignment::kNone) {
    snap_area_data.snap_axis = SnapAxis::kBoth;
  } else if (align.alignmentX != SnapAlignment::kNone &&
             align.alignmentY == SnapAlignment::kNone) {
    snap_area_data.snap_axis = SnapAxis::kX;
  } else {
    snap_area_data.snap_axis = SnapAxis::kY;
  }

  snap_area_data.must_snap =
      (area_style->ScrollSnapStop() == EScrollSnapStop::kAlways);

  return snap_area_data;
}

static ScrollSnapType GetSnappingType(ScrolledAxis scrolled_axis,
                                      ScrollSnapType type) {
  if (scrolled_axis == kScrolledAxisBoth)
    return type;
  if (scrolled_axis == kScrolledAxisX) {
    if (type.axis == SnapAxis::kX || type.axis == SnapAxis::kBoth) {
      type.axis = SnapAxis::kX;
      return type;
    }
  } else {
    if (type.axis == SnapAxis::kY || type.axis == SnapAxis::kBoth) {
      type.axis = SnapAxis::kY;
      return type;
    }
  }
  type.is_none = true;
  return type;
}

ScrollOffset SnapCoordinator::FindSnapOffset(ScrollOffset current_offset,
                                             const SnapContainerData& data) {
  float smallest_distance_x = FLT_MAX;
  float smallest_distance_y = FLT_MAX;
  float snap_offset_x = current_offset.Width();
  float snap_offset_y = current_offset.Height();
  for (SnapAreaData snap_area_data : data.snap_area_list) {
    // TODO(sunyunjia): We should consider visiblity when choosing snap offset.
    if (snap_area_data.snap_axis == SnapAxis::kX ||
        snap_area_data.snap_axis == SnapAxis::kBoth) {
      float offset = snap_area_data.snap_offset.x();
      float distance = std::abs(current_offset.Width() - offset);
      if (distance < smallest_distance_x) {
        smallest_distance_x = distance;
        snap_offset_x = offset;
      }
    }
    if (snap_area_data.snap_axis == SnapAxis::kY ||
        snap_area_data.snap_axis == SnapAxis::kBoth) {
      float offset = snap_area_data.snap_offset.y();
      float distance = std::abs(current_offset.Height() - offset);
      if (distance < smallest_distance_y) {
        smallest_distance_y = distance;
        snap_offset_y = offset;
      }
    }
  }
  return ScrollOffset(snap_offset_x, snap_offset_y);
}

bool SnapCoordinator::GetSnapPosition(const LayoutBox& snap_container,
                                      ScrolledAxis axis,
                                      ScrollOffset* snap_offset) {
  ScrollOffset current_scroll_offset =
      snap_container.GetScrollableArea()->GetScrollOffset();
  auto iter = snap_container_map_.find(&snap_container);
  if (iter == snap_container_map_.end())
    return false;

  SnapContainerData data = iter->value;
  ScrollSnapType type = GetSnappingType(axis, data.scroll_snap_type);
  if (!data.snap_area_list.size() || type.is_none)
    return false;

  *snap_offset = FindSnapOffset(current_scroll_offset, data);
  if (type.axis == SnapAxis::kX)
    snap_offset->SetHeight(current_scroll_offset.Height());
  if (type.axis == SnapAxis::kY)
    snap_offset->SetWidth(current_scroll_offset.Width());

  return true;
}

void SnapCoordinator::SnapContainerDidChange(LayoutBox& snap_container,
                                             ScrollSnapType scroll_snap_type) {
  if (scroll_snap_type.is_none) {
    snap_container_map_.erase(&snap_container);
    snap_container.ClearSnapAreas();
  } else {
    if (scroll_snap_type.axis == SnapAxis::kInline) {
      if (snap_container.Style()->IsHorizontalWritingMode())
        scroll_snap_type.axis = SnapAxis::kX;
      else
        scroll_snap_type.axis = SnapAxis::kY;
    }
    if (scroll_snap_type.axis == SnapAxis::kBlock) {
      if (snap_container.Style()->IsHorizontalWritingMode())
        scroll_snap_type.axis = SnapAxis::kY;
      else
        scroll_snap_type.axis = SnapAxis::kX;
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

SnapContainerData SnapCoordinator::CalculateSnapContainerData(
    LayoutBox* snap_container) {
  auto iter = snap_container_map_.find(snap_container);
  if (iter != snap_container_map_.end()) {
    return iter->value;
  }
  return SnapContainerData();
}

#ifndef NDEBUG

void SnapCoordinator::ShowSnapAreaMap() {
  for (auto& container : snap_container_map_.Keys())
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

void SnapCoordinator::PrintSnapData(const LayoutBox* snap_container) {
  auto iter = snap_container_map_.find(snap_container);
  if (iter != snap_container_map_.end()) {
    SnapContainerData data = iter->value;
    ScrollSnapType type = data.scroll_snap_type;
    for (SnapAreaData snap_area_data : data.snap_area_list) {
      LOG(INFO) << snap_area_data.snap_offset.x() << ", "
                << snap_area_data.snap_offset.y();
    }
  }
}

#endif

}  // namespace blink
