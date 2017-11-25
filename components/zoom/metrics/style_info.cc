// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/metrics/style_info.h"

namespace zoom {
namespace metrics {

StyleInfo::MainFrameStyleInfo::MainFrameStyleInfo() : preferred_width(0) {}
StyleInfo::MainFrameStyleInfo::~MainFrameStyleInfo() {}
StyleInfo::MainFrameStyleInfo::MainFrameStyleInfo(const MainFrameStyleInfo&) =
    default;

StyleInfo::StyleInfo() : text_area(0), image_area(0), object_area(0) {}
StyleInfo::~StyleInfo() {}
StyleInfo::StyleInfo(const StyleInfo&) = default;

bool StyleInfo::Merge(const StyleInfo& other) {
  if (main_frame_info && other.main_frame_info)
    return false;

  if (other.main_frame_info)
    main_frame_info = other.main_frame_info;

  for (const auto& other_font : other.font_size_distribution) {
    auto insertion =
        font_size_distribution.insert(std::make_pair(other_font.first, 0));
    insertion.first->second += other_font.second;
  }

  if (!largest_image ||
      (other.largest_image &&
       largest_image->GetArea() < other.largest_image->GetArea()))
    largest_image = other.largest_image;

  if (!largest_object ||
      (other.largest_object &&
       largest_object->GetArea() < other.largest_object->GetArea()))
    largest_object = other.largest_object;

  text_area += other.text_area;
  image_area += other.image_area;
  object_area += other.object_area;

  return true;
}

}  // namespace metrics
}  // namespace zoom
