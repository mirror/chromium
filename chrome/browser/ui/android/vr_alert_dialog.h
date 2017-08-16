// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_VR_ALERT_DIALOG_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_VR_ALERT_DIALOG_H_

#include <jni.h>

#include <map>
#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/callback.h"
#include "base/cancelable_callback.h"
#include "base/macros.h"

using base::android::JavaParamRef;

class VrAlertDialog {
 public:
  VrAlertDialog(JNIEnv* env,
                jobject obj,
                jlong native_vr_shell_delegate,
                jstring text_message);
  ~VrAlertDialog();

  void CreateVrAlertDialog(JNIEnv* env, jobject obj);

 private:
  base::android::ScopedJavaGlobalRef<jobject> vr_alert_dialog_;

  DISALLOW_COPY_AND_ASSIGN(VrAlertDialog);
};

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_VR_ALERT_DIALOG_H_
