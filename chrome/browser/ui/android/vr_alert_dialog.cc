// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/vr_alert_dialog.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/vr_shell/vr_shell_delegate.h"
#include "jni/VrAlertDialog_jni.h"

using base::android::JavaParamRef;

VrAlertDialog::VrAlertDialog(JNIEnv* env,
                             jobject obj,
                             jlong native_vr_shell_delegate,
                             jstring text_message) {
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

/*static void OnLibraryAvailable(JNIEnv* env, const JavaParamRef<jclass>& clazz)
{ device::GvrDelegateProvider::SetInstance(
      base::Bind(&VrShellDelegate::CreateVrShellDelegate));
}*/
