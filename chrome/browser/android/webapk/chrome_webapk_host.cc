// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/chrome_webapk_host.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/android/chrome_feature_list.h"
#include "jni/ChromeWebApkHost_jni.h"

namespace {

// Variations flag to enable launching Chrome renderer in WebAPK process.
const char* kLaunchRendererInWebApkProcess =
    "launch_renderer_in_webapk_process";

// Extra space allowed to install WebApk from play store.
const char* kWebapkExtraInstallationSpace =
    "webapk_extra_installation_space_mb";

}  // anonymous namespace

// static
bool ChromeWebApkHost::CanInstallWebApk() {
  return base::FeatureList::IsEnabled(chrome::android::kImprovedA2HS);
}

// static
jboolean JNI_ChromeWebApkHost_CanLaunchRendererInWebApkProcess(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  return base::GetFieldTrialParamValueByFeature(
             chrome::android::kImprovedA2HS, kLaunchRendererInWebApkProcess) ==
         "true";
}

// static
jlong JNI_ChromeWebApkHost_WebApkExtraInstallationSpace(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  int result_mb = 0;
  base::StringToInt(base::GetFieldTrialParamValueByFeature(
                        chrome::android::kAdjustWebApkInstallationSpace,
                        kWebapkExtraInstallationSpace),
                    &result_mb);
  return result_mb * 1024 * 1024;  // byte
}
