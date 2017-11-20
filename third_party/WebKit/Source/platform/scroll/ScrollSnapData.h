// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScrollSnapData_h
#define ScrollSnapData_h

#include "platform/wtf/Vector.h"
#include "ui/gfx/geometry/scroll_offset.h"

namespace blink {

enum class SnapAxis : unsigned {
  kBoth,
  kX,
  kY,
  kBlock,
  kInline,
};

enum class SnapStrictness : unsigned { kProximity, kMandatory };

enum class SnapAlignment : unsigned { kNone, kStart, kEnd, kCenter };

struct ScrollSnapType {
  DISALLOW_NEW();

  ScrollSnapType()
      : is_none(true),
        axis(SnapAxis::kBoth),
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
      : alignmentX(SnapAlignment::kNone), alignmentY(SnapAlignment::kNone) {}

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
  static const int kInvalidScrollOffset = -1;

  SnapAreaData() {}

  SnapAreaData(SnapAxis axis, gfx::ScrollOffset offset, bool msnap)
      : snap_axis(axis), snap_offset(offset), must_snap(msnap) {}

  SnapAreaData(const SnapAreaData& data)
      : snap_axis(data.snap_axis),
        snap_offset(data.snap_offset),
        must_snap(data.must_snap) {}

  SnapAreaData& operator=(const SnapAreaData& other) {
    if (&other == this)
      return *this;
    snap_axis = other.snap_axis;
    snap_offset = other.snap_offset;
    must_snap = other.must_snap;
    return *this;
  }

  // The axises along which the area has specified snap positions.
  SnapAxis snap_axis;

  // The scroll_offset to snap the area at the specified alignment in that axis.
  gfx::ScrollOffset snap_offset;

  // Whether this area has scroll-snap-stop: always.
  bool must_snap;

  // TODO(sunyunjia): Add fields for visibility requirement and large area
  // snapping.
};

struct SnapContainerData {
  SnapContainerData() : SnapContainerData(ScrollSnapType()) {}
  explicit SnapContainerData(ScrollSnapType type) : scroll_snap_type(type) {}
  SnapContainerData(ScrollSnapType type, gfx::ScrollOffset offset)
      : scroll_snap_type(type), max_offset(offset) {}
  void AddSnapAreaData(SnapAreaData snap_area_data) {
    snap_area_list.push_back(snap_area_data);
  }

  // The scroll-snap-type of the SnapContainer
  ScrollSnapType scroll_snap_type;

  // The maximal scrollable offset of the SnapContainer
  gfx::ScrollOffset max_offset;

  // The SnapAreaDatas in this SnapContainer.
  Vector<SnapAreaData> snap_area_list;
};

}  // namespace blink

#endif  // ScrollSnapData_h
