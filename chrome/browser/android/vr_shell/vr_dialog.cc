// Copyright 2017 The Chromium Authors. All rights reserved.
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

VrDialog::VrDialog(JNIEnv* env,
                   jobject obj,
                   jlong native_vr_shell_delegate,
                   int width,
                   int height) {
  java_vr_dialog_.Reset(env, obj);
  native_vr_shell_delegate_ = native_vr_shell_delegate;
  vr_dialog_.Reset(env, obj);
  reinterpret_cast<vr_shell::VrShellDelegate*>(native_vr_shell_delegate)
      ->AddAlertDialog(this, width, height);
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
                                jlong nativeVrShellDelegate,
                                jint width,
                                jint height) {
  return reinterpret_cast<intptr_t>(
      new VrDialog(env, obj, nativeVrShellDelegate, static_cast<int>(width),
                   static_cast<int>(height)));
}

void JNI_VrDialog_SetSize(JNIEnv* env,
                          const JavaParamRef<jobject>& obj,
                          jlong nativeVrShellDelegate,
                          jint width,
                          jint height) {
  reinterpret_cast<vr_shell::VrShellDelegate*>(nativeVrShellDelegate)
      ->SetAlertDialogSize(static_cast<int>(width), static_cast<int>(height));
}

void JNI_VrDialog_CloseVrDialog(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                jlong nativeVrShellDelegate) {
  reinterpret_cast<vr_shell::VrShellDelegate*>(nativeVrShellDelegate)
      ->CloseAlertDialog();
}

}  //  namespace vr_shell
