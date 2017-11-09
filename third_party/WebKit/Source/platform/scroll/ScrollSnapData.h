// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollSnapData_h
#define ScrollSnapData_h

#include "platform/wtf/Vector.h"

namespace blink {

enum SnapAxis {
  kSnapAxisBoth,
  kSnapAxisX,
  kSnapAxisY,
  kSnapAxisBlock,
  kSnapAxisInline,
};

enum class SnapStrictness { kProximity, kMandatory };

enum SnapAlignment {
  kSnapAlignmentNone,
  kSnapAlignmentStart,
  kSnapAlignmentEnd,
  kSnapAlignmentCenter
};

struct ScrollSnapType {
  DISALLOW_NEW();

  ScrollSnapType()
      : is_none(true),
        axis(kSnapAxisBoth),
        strictness(SnapStrictness::kProximity) {}

  ScrollSnapType(const ScrollSnapType& other)
      : is_none(other.is_none),
        axis(other.axis),
        strictness(other.strictness) {}

  ScrollSnapType(bool i, SnapAxis a, SnapStrictness s)
      : is_none(i), axis(a), strictness(s) {}

  bool operator==(const ScrollSnapType& other) const {
    return is_none == other.is_none && axis == other.axis &&
           strictness == other.strictness;
  }

  bool operator!=(const ScrollSnapType& other) const {
    return !(*this == other);
  }

  bool is_none;
  SnapAxis axis;
  SnapStrictness strictness;
};

struct ScrollSnapAlign {
  DISALLOW_NEW();

  ScrollSnapAlign()
      : alignmentX(kSnapAlignmentNone), alignmentY(kSnapAlignmentNone) {}

  ScrollSnapAlign(const ScrollSnapAlign& other)
      : alignmentX(other.alignmentX), alignmentY(other.alignmentY) {}

  explicit ScrollSnapAlign(SnapAlignment alignment)
      : alignmentX(alignment), alignmentY(alignment) {}

  ScrollSnapAlign(SnapAlignment x, SnapAlignment y)
      : alignmentX(x), alignmentY(y) {}

  bool operator==(const ScrollSnapAlign& other) const {
    return alignmentX == other.alignmentX && alignmentY == other.alignmentY;
  }

  bool operator!=(const ScrollSnapAlign& other) const {
    return !(*this == other);
  }

  SnapAlignment alignmentX;
  SnapAlignment alignmentY;
};

struct SnapAreaData {
  SnapAreaData() {}
  SnapAreaData(double sstart_x,
               double sstart_y,
               double send_x,
               double send_y,
               double vstart_x,
               double vstart_y,
               double vend_x,
               double vend_y,
               double msnap)
      : snap_start_x(sstart_x),
        snap_start_y(sstart_y),
        snap_end_x(send_x),
        snap_end_y(send_y),
        visible_start_x(vstart_x),
        visible_start_y(vstart_y),
        visible_end_x(vend_x),
        visible_end_y(vend_y),
        must_snap(msnap) {}
  // The minimal scroll_offset to align the element as specified.
  double snap_start_x;
  double snap_start_y;

  // The maximal scroll_offset to align the element as specified.
  double snap_end_x;
  double snap_end_y;

  // The minimal scroll_offset to ensure the element is visible.
  double visible_start_x;
  double visible_start_y;

  // The maximal scroll_offset to ensure the element is visible.
  double visible_end_x;
  double visible_end_y;

  // Whether this area has scroll-snap-stop: always.
  bool must_snap;
};

struct SnapContainerData {
  SnapContainerData() : SnapContainerData(ScrollSnapType()) {}
  explicit SnapContainerData(ScrollSnapType type) : scroll_snap_type(type) {}
  SnapContainerData(ScrollSnapType type, double sx, double sy)
      : scroll_snap_type(type), scrollable_x(sx), scrollable_y(sy) {}
  void AddSnapAreaData(SnapAreaData snap_area_data) {
    snap_area_list.push_back(snap_area_data);
  }

  // The scroll-snap-type of the SnapContainer
  ScrollSnapType scroll_snap_type;

  // The maximal scrollable offset of the SnapContainer
  double scrollable_x;
  double scrollable_y;

  // The SnapPoints in this SnapContainer.
  Vector<SnapAreaData> snap_area_list;
};

}  // namespace blink

#endif  // ScrollSnapData_h
