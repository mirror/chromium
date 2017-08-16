// Copyright 2016 The Chromium Authors. All rights reserved.
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
                             jstring text_message) {
  native_vr_shell_delegate_ = native_vr_shell_delegate;
  LOG(ERROR) << "VrAlertDialog::VrAlertDialog VRP: "
             << "Constructor Native ->"
             << base::android::ConvertJavaStringToUTF16(env, text_message);
  vr_alert_dialog_.Reset(env, obj);
  reinterpret_cast<vr_shell::VrShellDelegate*>(native_vr_shell_delegate)
      ->AddAlertDialog(
          base::android::ConvertJavaStringToUTF16(env, text_message));
}

VrAlertDialog::~VrAlertDialog() {
  DVLOG(1) << __FUNCTION__ << "=" << this;
}

void VrAlertDialog::CreateVrAlertDialog(JNIEnv* env, jobject obj) {
  // Just to make that it's not unused
  Java_VrAlertDialog_OnButtonClicked(env, vr_alert_dialog_ /*, whichButton*/);
}

void VrAlertDialog::SetButton(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj, jint which_button, jstring text) {
  // Just to make that it's not unused
   LOG(ERROR) << "VRP: " << "Native SetButton!";
   if (which_button != -1)
      return;
   reinterpret_cast<vr_shell::VrShellDelegate*>(native_vr_shell_delegate_)
      ->AddButton(
          base::android::ConvertJavaStringToUTF16(env, text));
 
//  Java_VrAlertDialog_OnButtonClicked(env, vr_alert_dialog_ /*, whichButton*/);
}


// ----------------------------------------------------------------------------
// Native JNI methods
// ----------------------------------------------------------------------------

jlong InitVrAlert(JNIEnv* env,
                  const JavaParamRef<jobject>& obj,
                  jlong nativeVrShellDelegate,
                  const base::android::JavaParamRef<jstring>& textMessage) {
  return reinterpret_cast<intptr_t>(
      new VrAlertDialog(env, obj, nativeVrShellDelegate, textMessage));
}

} //  namespace vr_shell
