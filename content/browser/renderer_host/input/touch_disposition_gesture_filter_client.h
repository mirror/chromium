// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_DISPOSITION_GESTURE_FILTER_CLIENT
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_DISPOSITION_GESTURE_FILTER_CLIENT

#include "content/common/content_export.h"

namespace blink {
class WebGestureEvent;
}  // namespace blink

namespace content {

class CONTENT_EXPORT TouchDispositionGestureFilterClient {
 public:
  virtual ~TouchDispositionGestureFilterClient() {}

  virtual bool FilterGestureEvent(blink::WebGestureEvent* gesture_event) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCH_DISPOSITION_GESTURE_FILTER_CLIENT
