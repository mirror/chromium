// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_LOCATION_PROVIDER_APK_ANDROID_H_
#define DEVICE_GEOLOCATION_LOCATION_PROVIDER_APK_ANDROID_H_

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/threading/thread_checker.h"
#include "device/geolocation/geoposition.h"
#include "device/geolocation/location_provider_android.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace device {

// Location provider for WebAPKs using the platform provider over JNI.
class LocationProviderApkAndroid : public LocationProviderAndroid {
 public:
  using OnGeopositionCallback = base::Callback<void(const Geoposition&)>;

  explicit LocationProviderApkAndroid(const std::string& package_name);
  ~LocationProviderApkAndroid() override;

  // LocationProviderAndroid implementation.
  bool StartProvider(bool high_accuracy) override;
  void StopProvider() override;

  // Called by JNI on its thread looper.
  void OnNewLocationAvailable(JNIEnv* env,
                              const base::android::JavaParamRef<jobject>& obj,
                              jdouble latitude,
                              jdouble longitude,
                              jdouble time_stamp,
                              jboolean has_altitude,
                              jdouble altitude,
                              jboolean has_accuracy,
                              jdouble accuracy,
                              jboolean has_heading,
                              jdouble heading,
                              jboolean has_speed,
                              jdouble speed);
  void OnNewErrorAvailable(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& obj,
                           jstring message);

 private:
  bool IsRunning();

  std::string package_name_;
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  base::ThreadChecker thread_checker_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_LOCATION_PROVIDER_APK_ANDROID_H_
