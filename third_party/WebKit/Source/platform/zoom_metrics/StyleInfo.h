// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StyleInfo_h
#define StyleInfo_h

#include "platform/PlatformExport.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/Optional.h"
#include "public/platform/WebSize.h"

namespace blink {
namespace zoom_metrics {

// Blink variant of blink.mojom.zoom_metrics.StyleInfo.
// See style_collector.mojom.
struct PLATFORM_EXPORT StyleInfo {
  StyleInfo();
  ~StyleInfo();

  struct MainFrameStyleInfo {
    MainFrameStyleInfo();
    ~MainFrameStyleInfo();

    WebSize visible_content_size;
    WebSize contents_size;
    uint32_t preferred_width;
  };

  WTF::HashMap<uint8_t, uint32_t> font_size_distribution;

  WTF::Optional<MainFrameStyleInfo> main_frame_info;

  WTF::Optional<WebSize> largest_image;
  WTF::Optional<WebSize> largest_object;

  uint32_t text_area;
  uint32_t image_area;
  uint32_t object_area;
};

}  // namespace zoom_metrics
}  // namespace blink

#endif  // StyleInfo_h
