// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_ANDROID_GESTURE_EVENT_ANDROID_H_
#define UI_EVENTS_ANDROID_GESTURE_EVENT_ANDROID_H_

#include <jni.h>
#include <vector>

#include "base/macros.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "ui/events/events_export.h"
#include "ui/gfx/geometry/point_conversions.h"

namespace gfx {
class PointF;
}

namespace ui {

// Event class used to carry the info from Java GestureEvent through native.
// All coordinates are in DIPs.
//
// TODO: For now this handles pinch gestures only. Extend to handle more types.
class EVENTS_EXPORT GestureEventAndroid {
 public:
  GestureEventAndroid(int type,
                      const gfx::PointF& location,
                      long time_ms,
                      float delta);

  ~GestureEventAndroid();

  int type() const { return type_; }
  const gfx::PointF& location() const { return location_; }
  long time() const { return time_ms_; }
  float delta() const { return delta_; }

  // Creates a new GestureEventAndroid instance different from |this| only by
  // its location.
  std::unique_ptr<GestureEventAndroid> CreateFor(
      const gfx::PointF& new_location) const;

 private:
  int type_;
  gfx::PointF location_;
  long time_ms_;
  float delta_;

  DISALLOW_COPY_AND_ASSIGN(GestureEventAndroid);
};

// Convenience method that converts an instance to blink event.
blink::WebGestureEvent CreateWebGestureEventFromGestureEventAndroid(
    const GestureEventAndroid& event);

}  // namespace ui

#endif  // UI_EVENTS_ANDROID_GESTURE_EVENT_ANDROID_H_
