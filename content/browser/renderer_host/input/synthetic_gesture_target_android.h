// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_GESTURE_TARGET_ANDROID_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_GESTURE_TARGET_ANDROID_H_

#include "content/browser/renderer_host/input/synthetic_gesture_target_base.h"
#include "content/public/browser/android/motion_event_action.h"

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"

namespace ui {
class LatencyInfo;
}  // namespace ui

namespace content {

// Owned by |SyntheticGestureController|. Keeps a weak pointer to Java object
// whose lifetime is that of WebContents.
class SyntheticGestureTargetAndroid : public SyntheticGestureTargetBase {
 public:
  SyntheticGestureTargetAndroid(JNIEnv* env,
                                const base::android::JavaParamRef<jobject>& obj,
                                RenderWidgetHostImpl* host,
                                float scale_factor);
  ~SyntheticGestureTargetAndroid() override;

  // SyntheticGestureTargetBase:
  void DispatchWebTouchEventToPlatform(
      const blink::WebTouchEvent& web_touch,
      const ui::LatencyInfo& latency_info) override;
  void DispatchWebMouseWheelEventToPlatform(
      const blink::WebMouseWheelEvent& web_wheel,
      const ui::LatencyInfo& latency_info) override;
  void DispatchWebMouseEventToPlatform(
      const blink::WebMouseEvent& web_mouse,
      const ui::LatencyInfo& latency_info) override;

  // SyntheticGestureTarget:
  SyntheticGestureParams::GestureSourceType
  GetDefaultSyntheticGestureSourceType() const override;
  float GetTouchSlopInDips() const override;
  float GetMinScalingSpanInDips() const override;

 private:
  void TouchSetPointer(int index, int x, int y, int id);
  void TouchSetScrollDeltas(int x, int y, int dx, int dy);
  void TouchInject(MotionEventAction action,
                   int pointer_count,
                   int64_t time_in_ms);

  float scale_factor_;
  JavaObjectWeakGlobalRef java_ref_;

  DISALLOW_COPY_AND_ASSIGN(SyntheticGestureTargetAndroid);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_GESTURE_TARGET_ANDROID_H_
