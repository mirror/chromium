// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_SYNTHETIC_GESTURE_TARGET_CONNECTOR_H_
#define CONTENT_BROWSER_ANDROID_SYNTHETIC_GESTURE_TARGET_CONNECTOR_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "content/public/browser/android/motion_event_action.h"

namespace ui {
class ViewAndroid;
}  // namespace ui

namespace content {

// Keeps a strong pointer to Java object.
class SyntheticGestureTargetConnector {
 public:
  explicit SyntheticGestureTargetConnector(ui::ViewAndroid* view);
  ~SyntheticGestureTargetConnector() override;

  void TouchSetPointer(int index, int x, int y, int id);
  void TouchSetScrollDeltas(int x, int y, int dx, int dy);
  void TouchInject(MotionEventAction action,
                   int pointer_count,
                   int64_t time_in_ms);

 private:
  ui::ViewAndroid* const view_;
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  DISALLOW_COPY_AND_ASSIGN(SyntheticGestureTargetConnector);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_SYNTHETIC_GESTURE_TARGET_CONNECTOR_H_
