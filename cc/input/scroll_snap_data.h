// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLL_SNAP_DATA_H_
#define CC_INPUT_SCROLL_SNAP_DATA_H_

#include <vector>
#include "cc/cc_export.h"

namespace cc {

enum SnapAxis {
  kSnapAxisBoth,
  kSnapAxisX,
  kSnapAxisY,
  kSnapAxisBlock,
  kSnapAxisInline,
};

enum SnapStrictness { kSnapStrictnessProximity, kSnapStrictnessMandatory };

enum SnapAlignment {
  kSnapAlignmentNone,
  kSnapAlignmentStart,
  kSnapAlignmentEnd,
  kSnapAlignmentCenter
};

struct ScrollSnapType {
  ScrollSnapType()
      : is_none(true),
        axis(kSnapAxisBoth),
        strictness(kSnapStrictnessProximity) {}

  ScrollSnapType(const ScrollSnapType& other)
      : is_none(other.is_none),
        axis(other.axis),
        strictness(other.strictness) {}

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
  ScrollSnapAlign()
      : alignmentX(kSnapAlignmentNone), alignmentY(kSnapAlignmentNone) {}

  ScrollSnapAlign(const ScrollSnapAlign& other)
      : alignmentX(other.alignmentX), alignmentY(other.alignmentY) {}

  bool operator==(const ScrollSnapAlign& other) const {
    return alignmentX == other.alignmentX && alignmentY == other.alignmentY;
  }

  bool operator!=(const ScrollSnapAlign& other) const {
    return !(*this == other);
  }

  SnapAlignment alignmentX;
  SnapAlignment alignmentY;
};

struct CC_EXPORT SnapPoint {
  SnapPoint();
  // The size and location of the SnapArea relative to its SnapContainer.
  double offset_left;
  double offset_top;
  double width;
  double height;
  // The scroll-snap-margin of the SnapArea
  double margin_left;
  double margin_top;
  double margin_right;
  double margin_bottom;
  // The scroll-snap-align of the SnapArea
  ScrollSnapAlign alignment;
  // Whether this area has scroll-snap-stop: always
  bool must_snap;
};

struct CC_EXPORT SnapData {
  SnapData();
  ~SnapData();
  // The scroll-snap-type of the SnapContainer
  ScrollSnapType scroll_snap_type;
  // The size of the SnapContainer.
  double client_width;
  double client_height;
  // The size of the content in the SnapContainer.
  double scroll_width;
  double scroll_height;
  // ScrollPadding of the SnapContainer
  double padding_left;
  double padding_top;
  double padding_right;
  double padding_bottom;
  // The SnapPoints in this SnapContainer
  std::vector<SnapPoint> snap_points;
};

}  // namespace cc

#endif // CC_INPUT_SCROLL_SNAP_DATA_H_
