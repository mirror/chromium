// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/vr_view.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/vr_shell/vr_shell_delegate.h"
#include "jni/VrView_jni.h"

using base::android::JavaParamRef;

namespace vr_shell {

VrView::VrView(JNIEnv* env, jobject obj, jlong native_vr_shell_delegate) {
  java_vr_view_.Reset(env, obj);
  native_vr_shell_delegate_ = native_vr_shell_delegate;
  vr_view_.Reset(env, obj);
  reinterpret_cast<vr_shell::VrShellDelegate*>(native_vr_shell_delegate)
      ->AddAlertDialog(this);
}

VrView::~VrView() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

void VrView::CreateVrView(JNIEnv* env, jobject obj) {}

void VrView::OnContentEnter(const gfx::PointF& normalized_hit_point) {}

void VrView::OnContentLeave() {}
void VrView::OnContentMove(const gfx::PointF& normalized_hit_point) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VrView_onMouseMove(env, java_vr_view_, normalized_hit_point.x(),
                          normalized_hit_point.y());
}

void VrView::OnContentDown(const gfx::PointF& normalized_hit_point) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VrView_onMouseDown(env, java_vr_view_, normalized_hit_point.x(),
                          normalized_hit_point.y());
}
void VrView::OnContentUp(const gfx::PointF& normalized_hit_point) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VrView_onMouseUp(env, java_vr_view_, normalized_hit_point.x(),
                        normalized_hit_point.y());
}
void VrView::OnContentFlingStart(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrView::OnContentFlingCancel(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrView::OnContentScrollBegin(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrView::OnContentScrollUpdate(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrView::OnContentScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                                const gfx::PointF& normalized_hit_point) {}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

jlong InitVrView(JNIEnv* env,
                 const JavaParamRef<jobject>& obj,
                 jlong nativeVrShellDelegate) {
  return reinterpret_cast<intptr_t>(
      new VrView(env, obj, nativeVrShellDelegate));
}

void CloseVrView(JNIEnv* env,
                 const JavaParamRef<jobject>& obj,
                 jlong nativeVrShellDelegate) {
  reinterpret_cast<vr_shell::VrShellDelegate*>(nativeVrShellDelegate)
      ->CloseAlertDialog();
}

}  //  namespace vr_shell
