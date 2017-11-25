// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ZOOM_METRICS_STYLE_INFO_H_
#define COMPONENTS_ZOOM_METRICS_STYLE_INFO_H_

#include <map>

#include "base/compiler_specific.h"
#include "base/optional.h"
#include "ui/gfx/geometry/size.h"

namespace zoom {
namespace metrics {

// Chromium variant of blink.mojom.zoom_metrics.StyleInfo.
// See style_collector.mojom.
struct StyleInfo {
  StyleInfo();
  ~StyleInfo();
  StyleInfo(const StyleInfo&);

  // Add in style info from |other|.
  // Returns true if the merge was valid.
  bool Merge(const StyleInfo& other) WARN_UNUSED_RESULT;

  struct MainFrameStyleInfo {
    MainFrameStyleInfo();
    ~MainFrameStyleInfo();
    MainFrameStyleInfo(const MainFrameStyleInfo&);

    gfx::Size visible_content_size;
    gfx::Size contents_size;
    uint32_t preferred_width;
  };

  std::map<uint8_t, uint32_t> font_size_distribution;

  base::Optional<MainFrameStyleInfo> main_frame_info;

  base::Optional<gfx::Size> largest_image;
  base::Optional<gfx::Size> largest_object;

  uint32_t text_area;
  uint32_t image_area;
  uint32_t object_area;
};

}  // namespace metrics
}  // namespace zoom

#endif  // COMPONENTS_ZOOM_METRICS_STYLE_INFO_H_
