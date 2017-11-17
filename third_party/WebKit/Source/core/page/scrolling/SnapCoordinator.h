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

  // Calculate the SnapContainerData for the specific snap container.
  SnapContainerData CalculateSnapContainerData(LayoutBox*);

  // Calculate the SnapAreaData for the specific snap area in its snap
  // container.
  SnapAreaData CalculateSnapAreaData(const LayoutBox&, const LayoutBox&);

  // Called by LocalFrameView::PerformPostLayoutTasks(), so that the snap data
  // are updated whenever a layout happens.
  void UpdateAllSnapContainerData();
  void UpdateSnapContainerData(const LayoutBox*);

  // Temporarily for debug purpose.
  void PrintSnapData(const LayoutBox*);

  // Called by ScrollManager::HandleGestureScrollEnd() to find the snap position
  // for the current scroll on the specific direction.
  ScrollOffset GetSnapPosition(const LayoutBox&, SnapAxis);
  static ScrollOffset FindSnapOffsetOnX(ScrollOffset, const SnapContainerData&);
  static ScrollOffset FindSnapOffsetOnY(ScrollOffset, const SnapContainerData&);
  static ScrollOffset FindSnapOffsetOnBoth(ScrollOffset,
                                           const SnapContainerData&);

#ifndef NDEBUG
  void ShowSnapAreaMap();
  void ShowSnapAreasFor(const LayoutBox*);
#endif

 private:
  friend class SnapCoordinatorTest;
  explicit SnapCoordinator();

  HashMap<const LayoutBox*, SnapContainerData> snap_container_map_;
};

}  // namespace blink

#endif  // SnapCoordinator_h
