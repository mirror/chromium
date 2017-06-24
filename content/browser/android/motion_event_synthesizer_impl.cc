// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/motion_event_synthesizer_impl.h"

#include "base/android/scoped_java_ref.h"
#include "base/memory/ptr_util.h"
#include "jni/MotionEventSynthesizer_jni.h"
#include "ui/android/view_android.h"

using base::android::JavaParamRef;

namespace content {

// static
std::unique_ptr<MotionEventSynthesizer> MotionEventSynthesizer::Create(
    ui::ViewAndroid* view) {
  return base::WrapUnique(new MotionEventSynthesizerImpl(view));
}

MotionEventSynthesizerImpl::MotionEventSynthesizerImpl(ui::ViewAndroid* view)
    : scale_factor_(view->GetDipScale()) {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_ref_ = JavaObjectWeakGlobalRef(
      env, Java_MotionEventSynthesizer_create(env, view->GetContainerView()));
}

MotionEventSynthesizerImpl::~MotionEventSynthesizerImpl() {}

void MotionEventSynthesizerImpl::SetPointer(int index, int x, int y, int id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_MotionEventSynthesizer_setPointer(env, obj, index, x * scale_factor_,
                                         y * scale_factor_, id);
}

void MotionEventSynthesizerImpl::SetScrollDeltas(int x, int y, int dx, int dy) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_MotionEventSynthesizer_setScrollDeltas(
      env, obj, x * scale_factor_, y * scale_factor_, dx * scale_factor_,
      dy * scale_factor_);
}

void MotionEventSynthesizerImpl::Inject(MotionEventAction action,
                                        int pointer_count,
                                        int64 time_ms) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;
  Java_MotionEventSynthesizer_inject(env, obj, action, pointer_count, time_ms);
}

}  // namespace content
