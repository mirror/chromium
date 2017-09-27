// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/location_provider_apk_android.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/geolocation/geoposition.h"
#include "jni/LocationProviderApkAndroid_jni.h"

namespace device {

// LocationProviderApkAndroid
LocationProviderApkAndroid::LocationProviderApkAndroid(
    const std::string& package_name)
    : LocationProviderAndroid(),
      package_name_(package_name),
      task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

LocationProviderApkAndroid::~LocationProviderApkAndroid() {
  LocationProviderAndroid::~LocationProviderAndroid();
}

bool LocationProviderApkAndroid::StartProvider(bool high_accuracy) {
  DCHECK(thread_checker_.CalledOnValidThread());

  JNIEnv* env = base::android::AttachCurrentThread();
  if (java_ref_.is_null()) {
    base::android::ScopedJavaLocalRef<jstring> java_package_name =
        base::android::ConvertUTF8ToJavaString(env, package_name_);
    java_ref_.Reset(Java_LocationProviderApkAndroid_create(
        env, reinterpret_cast<intptr_t>(this), java_package_name));
  }
  return Java_LocationProviderApkAndroid_start(env, java_ref_, high_accuracy);
}

void LocationProviderApkAndroid::StopProvider() {
  DCHECK(thread_checker_.CalledOnValidThread());

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_LocationProviderApkAndroid_stop(env, java_ref_);
  java_ref_.Reset();
}

bool LocationProviderApkAndroid::IsRunning() {
  DCHECK(thread_checker_.CalledOnValidThread());

  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_LocationProviderApkAndroid_isRunning(env, java_ref_);
}

void LocationProviderApkAndroid::OnNewLocationAvailable(
    JNIEnv* env,
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
    jdouble speed) {
  Geoposition position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.timestamp = base::Time::FromDoubleT(time_stamp);
  if (has_altitude)
    position.altitude = altitude;
  if (has_accuracy)
    position.accuracy = accuracy;
  if (has_heading)
    position.heading = heading;
  if (has_speed)
    position.speed = speed;

  task_runner_->PostTask(
      FROM_HERE, base::Bind(&LocationProviderApkAndroid::NotifyNewGeoposition,
                            base::Unretained(this), position));
}

// static
void LocationProviderApkAndroid::OnNewErrorAvailable(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jstring message) {
  Geoposition position_error;
  position_error.error_code = Geoposition::ERROR_CODE_POSITION_UNAVAILABLE;
  position_error.error_message =
      base::android::ConvertJavaStringToUTF8(env, message);

  task_runner_->PostTask(
      FROM_HERE, base::Bind(&LocationProviderApkAndroid::NotifyNewGeoposition,
                            base::Unretained(this), position_error));
}

}  // namespace device
