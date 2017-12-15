// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SnapCoordinator_h
#define SnapCoordinator_h

#include "core/CoreExport.h"
#include "core/css/CSSPrimitiveValueMappings.h"
#include "platform/heap/Handle.h"

namespace blink {

class LayoutBox;
struct ScrollSnapType;
struct ScrollSnapAlign;
struct SnapAreaData;
struct SnapContainerData;

// Snap Coordinator keeps track of snap containers and all of their associated
// snap areas. It also contains the logic to generate the list of valid snap
// positions for a given snap container.
//
// Snap container:
//   An scroll container that has 'scroll-snap-type' value other
//   than 'none'.
// Snap area:
//   A snap container's descendant that contributes snap positions. An element
//   only contributes snap positions to its nearest ancestor (on the element’s
//   containing block chain) scroll container.
//
// For more information see spec: https://drafts.csswg.org/css-snappoints/
class CORE_EXPORT SnapCoordinator final
    : public GarbageCollectedFinalized<SnapCoordinator> {
  WTF_MAKE_NONCOPYABLE(SnapCoordinator);

 public:
  static SnapCoordinator* Create();
  ~SnapCoordinator();
  void Trace(blink::Visitor* visitor) {}

  void SnapContainerDidChange(LayoutBox&, ScrollSnapType);
  void SnapAreaDidChange(LayoutBox&, ScrollSnapAlign);

  // Returns the SnapContainerData for the specific snap container.
  SnapContainerData EnsureSnapContainerData(const LayoutBox& snap_container);

  // Calculate the SnapAreaData for the specific snap area in its snap
  // container.
  SnapAreaData CalculateSnapAreaData(const LayoutBox& snap_area,
                                     const LayoutBox& snap_container,
                                     const ScrollOffset& min_offset,
                                     const ScrollOffset& max_offset);

  // Called by LocalFrameView::PerformPostLayoutTasks(), so that the snap data
  // are updated whenever a layout happens.
  void UpdateAllSnapContainerData();
  void UpdateSnapContainerData(const LayoutBox&);

  // Called by ScrollManager::HandleGestureScrollEnd() to animate to the snap
  // position for the current scroll on the specific direction if there is
  // a valid snap position.
  void PerformSnapping(const LayoutBox& snap_container,
                       bool did_scroll_x,
                       bool did_scroll_y);
  bool GetSnapPosition(const LayoutBox& snap_container,
                       bool did_scroll_x,
                       bool did_scroll_y,
                       ScrollOffset* snap_offset);

  // Returns true if there is a valid offset to snap to. If the snap area it
  // finds on x and the one it finds on y could not be visible at same time,
  // it chooses the closer snap area and recursively calls itself to find
  // another snap offset on the other axis that satisfies the visilbility
  // requirement of the first snap area.
  // If such a snap offset does not exist, we return the original offset.
  // current_offset - The original scroll offset before snapping.
  // snap_container_data - The snap data of the snap container that we'd like to
  //    perform snapping.
  // visibility_requirement - The snap_offset we choose should be within this
  //    rect. The rect is the scrollable region of the container if this is the
  //    first call. It is the visible area of the snapped area if this is a
  //    recursive call after we have already snapped on one of the axes.
  // should_snap_on_x, should_snap_on_y - Whether we should perform snapping on
  //    each axis. They should be set to the scrolled axes if this is the first
  //    call. They should be set to the non-snapped axis if this is a recursive
  //    call after we have already snapped on one of the axes.
  // snap_offset - If this is the first call, snap_offset has the same value as
  //    current_offset. If this is a recursive call after we have already
  //    snapped on an axis, snap_offset would have the snapped value on that
  //    axis. This is to make sure the final snap_offset we choose is visible
  //    given the snapped offset. This parameter is also responsible to return
  //    the final snap_offset.
  static bool FindSnapOffset(const ScrollOffset& current_offset,
                             const SnapContainerData&,
                             const FloatRect& visibility_requirement,
                             bool should_snap_on_x,
                             bool should_snap_on_y,
                             ScrollOffset* snap_offset);

#ifndef NDEBUG
  void ShowSnapAreaMap();
  void ShowSnapAreasFor(const LayoutBox*);
  void ShowSnapDataFor(const LayoutBox*);
#endif

 private:
  friend class SnapCoordinatorTest;
  explicit SnapCoordinator();

  HashMap<const LayoutBox*, SnapContainerData> snap_container_map_;
};

}  // namespace blink

#endif  // SnapCoordinator_h
