// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/vr_alert_dialog.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/vr_shell/vr_shell_delegate.h"
#include "jni/VrAlertDialog_jni.h"

using base::android::JavaParamRef;

namespace vr_shell {

VrAlertDialog::VrAlertDialog(JNIEnv* env,
                             jobject obj,
                             jlong native_vr_shell_delegate,
                             jlong icon,
                             jstring title_text,
                             jstring toggle_text,
                             jint button_positive,
                             jstring button_positive_text,
                             jint button_negative,
                             jstring button_negative_text) {
  native_vr_shell_delegate_ = native_vr_shell_delegate;
  vr_alert_dialog_.Reset(env, obj);
  reinterpret_cast<vr_shell::VrShellDelegate*>(native_vr_shell_delegate)
      ->AddAlertDialog(
          icon, base::android::ConvertJavaStringToUTF16(env, title_text),
          base::android::ConvertJavaStringToUTF16(env, toggle_text),
          button_positive,
          base::android::ConvertJavaStringToUTF16(env, button_positive_text),
          button_negative,
          base::android::ConvertJavaStringToUTF16(env, button_negative_text));
}

VrAlertDialog::~VrAlertDialog() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

void VrAlertDialog::CreateVrAlertDialog(JNIEnv* env, jobject obj) {
  // Just to make sure that it's not unused
  Java_VrAlertDialog_OnButtonClicked(env, vr_alert_dialog_ /*, whichButton*/);
}

// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

jlong InitVrAlert(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jlong nativeVrShellDelegate,
    jlong icon,
    const base::android::JavaParamRef<jstring>& title_text,
    const base::android::JavaParamRef<jstring>& toggle_text,
    jint button_positive,
    const base::android::JavaParamRef<jstring>& button_positive_text,
    jint button_negative,
    const base::android::JavaParamRef<jstring>& button_negative_text) {
  return reinterpret_cast<intptr_t>(
      new VrAlertDialog(env, obj, nativeVrShellDelegate, icon, title_text,
                        toggle_text, button_positive, button_positive_text,
                        button_negative, button_negative_text));
}

}  //  namespace vr_shell
