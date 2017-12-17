// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scroll_snap_data.h"

namespace cc {

SnapContainerData::SnapContainerData() : SnapContainerData(ScrollSnapType()) {}

SnapContainerData::SnapContainerData(ScrollSnapType type)
    : scroll_snap_type(type) {}

SnapContainerData::SnapContainerData(ScrollSnapType type, gfx::ScrollOffset max)
    : scroll_snap_type(type), max_offset(max) {}

SnapContainerData::SnapContainerData(const SnapContainerData& other)
    : scroll_snap_type(other.scroll_snap_type),
      max_offset(other.max_offset),
      snap_area_list(other.snap_area_list) {}

SnapContainerData::~SnapContainerData() {}

void SnapContainerData::AddSnapAreaData(SnapAreaData snap_area_data) {
  snap_area_list.push_back(snap_area_data);
}

std::ostream& operator<<(std::ostream& ostream, const SnapAreaData& area_data) {
  return ostream << area_data.snap_offset.x() << ", "
                 << area_data.snap_offset.y();
}

std::ostream& operator<<(std::ostream& ostream,
                         const SnapContainerData& container_data) {
  for (SnapAreaData area_data : container_data.snap_area_list) {
    ostream << area_data << "\n";
  }
  return ostream;
}

}  // namespace cc
