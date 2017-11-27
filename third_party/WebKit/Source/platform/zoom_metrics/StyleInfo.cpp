// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/zoom_metrics/StyleInfo.h"

namespace blink {
namespace zoom_metrics {

StyleInfo::MainFrameStyleInfo::MainFrameStyleInfo() : preferred_width(0) {}
StyleInfo::MainFrameStyleInfo::~MainFrameStyleInfo() {}

StyleInfo::StyleInfo() : text_area(0), image_area(0), object_area(0) {}
StyleInfo::~StyleInfo() {}

}  // namespace zoom_metrics
}  // namespace blink
