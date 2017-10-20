// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_ANDROID_BATTERY_STATUS_LISTENER_ANDROID_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_ANDROID_BATTERY_STATUS_LISTENER_ANDROID_H_

#include "components/download/internal/scheduler/device_status_listener.h"

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

namespace download {

// Backed by a Java class that holds helper functions to query battery status.
class BatteryStatusListenerAndroid : public BatteryStatusListener {
 public:
  BatteryStatusListenerAndroid();
  ~BatteryStatusListenerAndroid() override;

  // BatteryStatusListener implementation.
  int GetBatteryPercentage() override;

 private:
  // The Java side object owned by this class.
  base::android::ScopedJavaGlobalRef<jobject> java_obj_;

  DISALLOW_COPY_AND_ASSIGN(BatteryStatusListenerAndroid);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_ANDROID_BATTERY_STATUS_LISTENER_ANDROID_H_
