// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/android/battery_status_listener_android.h"

#include "base/android/jni_android.h"
#include "jni/BatteryStatusListenerAndroid_jni.h"

namespace download {

BatteryStatusListenerAndroid::BatteryStatusListenerAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_obj_.Reset(env, Java_BatteryStatusListenerAndroid_create(
                           env, reinterpret_cast<intptr_t>(this))
                           .obj());
}

BatteryStatusListenerAndroid::~BatteryStatusListenerAndroid() {
  Java_BatteryStatusListenerAndroid_clearNativePtr(
      base::android::AttachCurrentThread(), java_obj_);
}

int BatteryStatusListenerAndroid::GetBatteryPercentage() {
  return Java_BatteryStatusListenerAndroid_getBatteryPercentage(
      base::android::AttachCurrentThread(), java_obj_);
}

}  // namespace download
