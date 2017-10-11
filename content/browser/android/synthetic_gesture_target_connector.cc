// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/synthetic_gesture_target_connector.h"

#include "base/trace_event/trace_event.h"
#include "jni/SyntheticGestureTarget_jni.h"
#include "ui/android/view_android.h"
#include "ui/gfx/android/view_configuration.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

SyntheticGestureTargetConnector::SyntheticGestureTargetConnector(
    ui::ViewAndroid* view)
    : view_(view) {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_ref_.Reset(
      Java_SyntheticGestureTarget_create(env, view->GetContainerView()));
}

SyntheticGestureTargetConnector::~SyntheticGestureTargetConnector() = default;

void SyntheticGestureTargetConnector::TouchSetPointer(int index,
                                                      int x,
                                                      int y,
                                                      int id) {
  TRACE_EVENT0("input", "SyntheticGestureTargetAndroid::TouchSetPointer");
  JNIEnv* env = base::android::AttachCurrentThread();
  float scale_factor = view_->GetDipScale();
  Java_SyntheticGestureTarget_setPointer(
      env, java_ref_, index, x * scale_factor, y * scale_factor, id);
}

void SyntheticGestureTargetConnector::TouchSetScrollDeltas(int x,
                                                           int y,
                                                           int dx,
                                                           int dy) {
  TRACE_EVENT0("input", "SyntheticGestureTargetAndroid::TouchSetScrollDeltas");
  JNIEnv* env = base::android::AttachCurrentThread();
  float scale_factor = view_->GetDipScale();
  Java_SyntheticGestureTarget_setScrollDeltas(
      env, java_ref_, x * scale_factor, y * scale_factor, dx * scale_factor,
      dy * scale_factor);
}

void SyntheticGestureTargetConnector::TouchInject(MotionEventAction action,
                                                  int pointer_count,
                                                  int64_t time_in_ms) {
  TRACE_EVENT0("input", "SyntheticGestureTargetAndroid::TouchInject");
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_SyntheticGestureTarget_inject(env, java_ref_, action, pointer_count,
                                     time_in_ms);
}

}  // namespace content
