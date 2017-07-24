// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "SnapCoordinator.h"

#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutView.h"
#include "core/style/ComputedStyle.h"
#include "platform/LengthFunctions.h"
#include "public/platform/WebSnapPointList.h"

namespace blink {

SnapCoordinator::SnapCoordinator() : snap_containers_() {}

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
  if (scroll_snap_align.alignmentX == kSnapAlignmentNone &&
      scroll_snap_align.alignmentY == kSnapAlignmentNone) {
    snap_area.SetSnapContainer(nullptr);
    return;
  }

  if (LayoutBox* snap_container = FindSnapContainer(snap_area)) {
    snap_area.SetSnapContainer(snap_container);
  } else {
    // TODO(majidvp): keep track of snap areas that do not have any
    // container so that we check them again when a new container is
    // added to the page.
  }
}

void SnapCoordinator::SnapContainerDidChange(LayoutBox& snap_container,
                                             ScrollSnapType scroll_snap_type) {
  if (scroll_snap_type.is_none) {
    // TODO(majidvp): Track and report these removals to CompositorWorker
    // instance responsible for snapping
    snap_containers_.erase(&snap_container);
    snap_container.ClearSnapAreas();
  } else {
    snap_containers_.insert(&snap_container);
  }

  // TODO(majidvp): Add logic to correctly handle orphaned snap areas here.
  // 1. Removing container: find a new snap container for its orphan snap
  // areas (most likely nearest ancestor of current container) otherwise add
  // them to an orphan list.
  // 2. Adding container: may take over snap areas from nearest ancestor snap
  // container or from existing areas in orphan pool.
}

static float ClipInContainer(float point, float range) {
  if (point < 0)
    return 0;
  return point > range ? range : point;
}

static void AddSnapArea(const LayoutBox& container_box,
                        const LayoutBox& snap_area,
                        SnapOrientation orientation,
                        WebSnapPointList* list) {
  SnapAlignment alignment;
  // The size of the container to snap the elements on the snapping axis.
  // Should be the original container_box size minus the paddings.
  float container_size = 0;
  // The total size of the content inside the container on the snapping axis.
  float container_content = 0;
  // The size of the container that is visible to users on the snapping axis.
  // Should be the original contianer_box size.
  float container_visible = 0;
  // The total size of the content inside the container on the non-snapping
  // axis.
  float area_size = 0;
  // The size of the snap_area that is visible to users on the snapping axis.
  // Should be the original snap_area size.
  float area_visible = 0;
  // The size of the snap_area that is visible to users on the non-snapping
  // axis. Should be the original snap_area size.
  float area_offset = 0;
  // The offset_range on the non-snapping axis to ensure that the snap_area's
  // minimum visibility.
  float block_visible_start = 0;
  float block_visible_end = 0;

  const ComputedStyle* area_style = snap_area.Style();
  const ComputedStyle* container_style = container_box.Style();
  bool must_snap = (area_style->ScrollSnapStop() == EScrollSnapStop::kAlways);
  if (orientation == kSnapX) {
    alignment = area_style->GetScrollSnapAlign().alignmentX;
    container_size = container_box.ClientWidth().ToFloat();
    container_visible = container_size;
    container_content = container_box.ScrollWidth().ToFloat();
    Length pad_left = container_style->ScrollPaddingLeft();
    Length pad_right = container_style->ScrollPaddingRight();
    float padding_left = pad_left.IsPercent()
                             ? pad_left.GetFloatValue() * container_size / 100
                             : pad_left.GetFloatValue();
    float padding_right = pad_right.IsPercent()
                              ? pad_right.GetFloatValue() * container_size / 100
                              : pad_right.GetFloatValue();
    container_size -= padding_left + padding_right;

    area_offset =
        snap_area.OffsetLeft(ToElement(container_box.GetNode())).ToFloat();
    area_offset -=
        area_style->ScrollSnapMarginLeft().GetFloatValue() + padding_left;
    area_size = snap_area.OffsetWidth().ToFloat();
    area_visible = area_size;
    area_size += area_style->ScrollSnapMarginLeft().GetFloatValue() +
                 area_style->ScrollSnapMarginRight().GetFloatValue();

    float container_visible_block = container_box.ClientHeight().ToFloat();
    float container_content_block = container_box.ScrollHeight().ToFloat();
    float area_block = snap_area.OffsetHeight().ToFloat();
    float offset_block =
        snap_area.OffsetTop(ToElement(container_box.GetNode())).ToFloat();
    block_visible_start =
        ClipInContainer(offset_block - container_visible_block,
                        container_content_block - container_visible_block);
    block_visible_end =
        ClipInContainer(offset_block + area_block,
                        container_content_block - container_visible_block);
  } else {
    alignment = area_style->GetScrollSnapAlign().alignmentY;
    container_size = container_box.ClientHeight().ToFloat();
    container_visible = container_size;
    container_content = container_box.ScrollHeight().ToFloat();
    Length pad_top = container_style->ScrollPaddingTop();
    Length pad_bottom = container_style->ScrollPaddingBottom();
    float padding_top = pad_top.IsPercent()
                            ? pad_top.GetFloatValue() * container_size / 100
                            : pad_top.GetFloatValue();
    float padding_bottom = pad_bottom.IsPercent() ? pad_bottom.GetFloatValue() *
                                                        container_size / 100
                                                  : pad_bottom.GetFloatValue();
    container_size -= padding_top + padding_bottom;

    area_offset =
        snap_area.OffsetTop(ToElement(container_box.GetNode())).ToFloat();
    area_offset -=
        area_style->ScrollSnapMarginTop().GetFloatValue() + padding_top;
    area_size = snap_area.OffsetHeight().ToFloat();
    area_visible = area_size;
    area_size += area_style->ScrollSnapMarginTop().GetFloatValue() +
                 area_style->ScrollSnapMarginBottom().GetFloatValue();

    float container_visible_block = container_box.ClientWidth().ToFloat();
    float container_content_block = container_box.ScrollWidth().ToFloat();
    float area_block = snap_area.OffsetWidth().ToFloat();
    float offset_block =
        snap_area.OffsetLeft(ToElement(container_box.GetNode())).ToFloat();
    block_visible_start =
        ClipInContainer(offset_block - container_visible_block,
                        container_content_block - container_visible_block);
    block_visible_end =
        ClipInContainer(offset_block + area_block,
                        container_content_block - container_visible_block);
  }
  if (alignment == kSnapAlignmentNone)
    return;
  // The range of the offset that is the container can be scrolled to.
  float scroll_range = container_content - container_visible;
  float start_alignment_offset = ClipInContainer(area_offset, scroll_range);
  float center_alignment_offset = ClipInContainer(
      area_offset + (area_size - container_size) / 2, scroll_range);
  float end_alignment_offset =
      ClipInContainer(area_offset + area_size - container_size, scroll_range);
  // If the snap_area is larger than the snap_container, every offset within
  // the range of start_alignment_offset and end_alignment_offset is a valid
  // snap_offset.
  if (end_alignment_offset > start_alignment_offset) {
    list->AddSnapPoint(orientation, start_alignment_offset, block_visible_start,
                       block_visible_end, must_snap, true, false);
    list->AddSnapPoint(orientation, end_alignment_offset, block_visible_start,
                       block_visible_end, must_snap, false, true);
    return;
  }
  if (alignment == kSnapAlignmentStart) {
    list->AddSnapPoint(orientation, start_alignment_offset, block_visible_start,
                       block_visible_end, must_snap);
    return;
  }
  if (alignment == kSnapAlignmentCenter) {
    list->AddSnapPoint(orientation, center_alignment_offset,
                       block_visible_start, block_visible_end, must_snap);
    return;
  }
  if (alignment == kSnapAlignmentEnd) {
    list->AddSnapPoint(orientation, end_alignment_offset, block_visible_start,
                       block_visible_end, must_snap);
    return;
  }

  NOTREACHED();
  return;
}

WebSnapPointList SnapCoordinator::SnapOffsets(const ContainerNode& element) {
  const ComputedStyle* style = element.GetComputedStyle();
  const LayoutBox* snap_container = element.GetLayoutBox();
  DCHECK(style);
  DCHECK(snap_container);

  WebSnapPointList result;
  SnapAxis axis = style->GetScrollSnapType().axis;
  bool snap_on_x =
      (axis == kSnapAxisX || axis == kSnapAxisBoth ||
       (axis == kSnapAxisInline && style->IsHorizontalWritingMode()) ||
       (axis == kSnapAxisBlock && !style->IsHorizontalWritingMode()));
  bool snap_on_y =
      (axis == kSnapAxisY || axis == kSnapAxisBoth ||
       (axis == kSnapAxisBlock && style->IsHorizontalWritingMode()) ||
       (axis == kSnapAxisInline && !style->IsHorizontalWritingMode()));

  if (SnapAreaSet* snap_areas = snap_container->SnapAreas()) {
    for (auto& snap_area : *snap_areas) {
      if (snap_on_x)
        AddSnapArea(*snap_container, *snap_area, kSnapX, &result);
      if (snap_on_y)
        AddSnapArea(*snap_container, *snap_area, kSnapY, &result);
    }
  }

  result.Sort();
  return result;
}

#ifndef NDEBUG

void SnapCoordinator::ShowSnapAreaMap() {
  for (auto& container : snap_containers_)
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
