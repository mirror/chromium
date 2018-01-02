// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scroll_snap_data.h"
#include "base/vlog.h"

namespace cc {

SnapContainerData::SnapContainerData() : SnapContainerData(ScrollSnapType()) {}

SnapContainerData::SnapContainerData(ScrollSnapType type)
    : scroll_snap_type(type) {}

SnapContainerData::SnapContainerData(ScrollSnapType type, gfx::ScrollOffset max)
    : scroll_snap_type(type), max_position(max) {}

SnapContainerData::SnapContainerData(const SnapContainerData& other)
    : scroll_snap_type(other.scroll_snap_type),
      max_position(other.max_position),
      snap_area_list(other.snap_area_list) {}

SnapContainerData::~SnapContainerData() {}

void SnapContainerData::AddSnapAreaData(SnapAreaData snap_area_data) {
  snap_area_list.push_back(snap_area_data);
}

gfx::ScrollOffset SnapContainerData::FindSnapPosition(
    const gfx::ScrollOffset& current_position,
    bool should_snap_on_x,
    bool should_snap_on_y) {
//  LOG(ERROR) << current_position.ToString();
  SnapAxis axis = scroll_snap_type.axis;
  should_snap_on_x &= (axis == SnapAxis::kX || axis == SnapAxis::kBoth);
  should_snap_on_y &= (axis == SnapAxis::kY || axis == SnapAxis::kBoth);
  if (!should_snap_on_x && !should_snap_on_y)
    return current_position;

  float smallest_distance_x = std::numeric_limits<float>::max();
  float smallest_distance_y = std::numeric_limits<float>::max();

  gfx::ScrollOffset snap_position = current_position;

  for (SnapAreaData snap_area_data : snap_area_list) {
    // TODO(sunyunjia): We should consider visiblity when choosing snap offset.
    if (should_snap_on_x && (snap_area_data.snap_axis == SnapAxis::kX ||
                             snap_area_data.snap_axis == SnapAxis::kBoth)) {
      float offset = snap_area_data.snap_position.x();
      if (offset == SnapAreaData::kInvalidScrollPosition)
        continue;
      float distance = std::abs(current_position.x() - offset);
      if (distance < smallest_distance_x) {
        smallest_distance_x = distance;
        snap_position.set_x(offset);
      }
    }
    if (should_snap_on_y && (snap_area_data.snap_axis == SnapAxis::kY ||
                             snap_area_data.snap_axis == SnapAxis::kBoth)) {
      float offset = snap_area_data.snap_position.y();
      if (offset == SnapAreaData::kInvalidScrollPosition)
        continue;
      float distance = std::abs(current_position.y() - offset);
      if (distance < smallest_distance_y) {
        smallest_distance_y = distance;
        snap_position.set_y(offset);
      }
    }
  }
//  LOG(ERROR) << "snap_position: " << snap_position.ToString();
  return snap_position;
}

static gfx::ScrollOffset ExpectedDistance(const gfx::ScrollOffset& delta) {
  gfx::ScrollOffset expected_distance = delta;
  expected_distance.Scale(11);
  return expected_distance;
}

static gfx::ScrollOffset ScaledDelta(const gfx::ScrollOffset& delta,
                                     const gfx::ScrollOffset& distance,
                                     const gfx::ScrollOffset& expected) {
  float x_scale = 0, y_scale = 0;
  if (distance.x() != 0)
    x_scale = expected.x() / distance.x();
  if (distance.y() != 0)
    y_scale = expected.y() / distance.y();

  gfx::ScrollOffset scaled_delta = delta;
  scaled_delta.Scale(x_scale, y_scale);
  if (std::abs(scaled_delta.x()) < 1 && scaled_delta.x() != 0)
    scaled_delta.set_x(scaled_delta.x() > 0 ? 1 : -1);
  if (std::abs(scaled_delta.y()) < 1 && scaled_delta.y() != 0)
    scaled_delta.set_y(scaled_delta.y() > 0 ? 1 : -1);

  return scaled_delta;
}

gfx::ScrollOffset SnapContainerData::AdjustFlingDelta(
    const gfx::ScrollOffset& current_position,
    const gfx::ScrollOffset& delta) {
  const float kEpsilon = 0.1f;
//  LOG(ERROR) << current_position.ToString() << "..";
  gfx::ScrollOffset expected_distance = ExpectedDistance(delta);
  gfx::ScrollOffset snap_position =
      FindSnapPosition(current_position + expected_distance,
                       std::abs(delta.x()) > kEpsilon, std::abs(delta.y()) > kEpsilon);
  return ScaledDelta(delta, expected_distance,
                     snap_position - current_position);
}

std::ostream& operator<<(std::ostream& ostream, const SnapAreaData& area_data) {
  return ostream << area_data.snap_position.x() << ", "
                 << area_data.snap_position.y();
}

std::ostream& operator<<(std::ostream& ostream,
                         const SnapContainerData& container_data) {
  for (SnapAreaData area_data : container_data.snap_area_list) {
    ostream << area_data << "\n";
  }
  return ostream;
}

}  // namespace cc
