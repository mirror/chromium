// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/vr_dialog.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/vr_shell/vr_shell_delegate.h"
#include "jni/VrDialog_jni.h"

using base::android::JavaParamRef;

namespace vr_shell {

VrDialog::VrDialog(JNIEnv* env, jobject obj, int width, int height) {
  java_vr_dialog_.Reset(env, obj);
  vr_dialog_.Reset(env, obj);
}

VrDialog::~VrDialog() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

void VrDialog::CreateVrDialog(JNIEnv* env, jobject obj) {}

void VrDialog::OnContentEnter(const gfx::PointF& normalized_hit_point) {}

void VrDialog::OnContentLeave() {}
void VrDialog::OnContentMove(const gfx::PointF& normalized_hit_point) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VrDialog_onMouseMove(env, java_vr_dialog_, normalized_hit_point.x(),
                            normalized_hit_point.y());
}

void VrDialog::OnContentDown(const gfx::PointF& normalized_hit_point) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VrDialog_onMouseDown(env, java_vr_dialog_, normalized_hit_point.x(),
                            normalized_hit_point.y());
}
void VrDialog::OnContentUp(const gfx::PointF& normalized_hit_point) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_VrDialog_onMouseUp(env, java_vr_dialog_, normalized_hit_point.x(),
                          normalized_hit_point.y());
}
void VrDialog::OnContentFlingStart(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrDialog::OnContentFlingCancel(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrDialog::OnContentScrollBegin(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrDialog::OnContentScrollUpdate(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}
void VrDialog::OnContentScrollEnd(
    std::unique_ptr<blink::WebGestureEvent> gesture,
    const gfx::PointF& normalized_hit_point) {}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

jlong JNI_VrDialog_InitVrDialog(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                jint width,
                                jint height) {
  return reinterpret_cast<intptr_t>(new VrDialog(
      env, obj, static_cast<int>(width), static_cast<int>(height)));
}

}  //  namespace vr_shell
