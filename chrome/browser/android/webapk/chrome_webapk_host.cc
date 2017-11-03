// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/webapk/chrome_webapk_host.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "chrome/browser/android/chrome_feature_list.h"
#include "chrome/browser/android/preferences/pref_service_bridge.h"
#include "components/variations/variations_associated_data.h"
#include "jni/ChromeWebApkHost_jni.h"

namespace {

// Variations flag to enable launching Chrome renderer in WebAPK process.
const char* kLaunchRendererInWebApkProcess =
    "launch_renderer_in_webapk_process";

}  // anonymous namespace

// static
bool ChromeWebApkHost::CanInstallWebApk() {
  return base::FeatureList::IsEnabled(chrome::android::kImprovedA2HS);
}

// static
ContentSetting ChromeWebApkHost::GetPermissionStatus(
    const std::string& package_name,
    ContentSetting content_setting,
    bool is_main_frame,
    std::vector<ContentSettingsType> content_settings_types) {
  DCHECK(!package_name.empty());

  if (!is_main_frame && content_setting != CONTENT_SETTING_ALLOW)
    return CONTENT_SETTING_BLOCK;

  JNIEnv* env = base::android::AttachCurrentThread();
  for (ContentSettingsType content_settings_type : content_settings_types) {
    std::vector<std::string> android_permissions;
    PrefServiceBridge::GetAndroidPermissionsForContentSetting(
        content_settings_type, &android_permissions);
    if (android_permissions.empty())
      continue;

    for (const auto& android_permission : android_permissions) {
      if (!Java_ChromeWebApkHost_hasPermission(
              env, base::android::ConvertUTF8ToJavaString(env, package_name),
              base::android::ConvertUTF8ToJavaString(env,
                                                     android_permission))) {
        return CONTENT_SETTING_ASK;
      }
    }
  }
  return CONTENT_SETTING_ALLOW;
}

// static
bool ChromeWebApkHost::HasAndroidLocationPermission(
    const std::string& package_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_ChromeWebApkHost_hasAndroidLocationPermission(
      env, base::android::ConvertUTF8ToJavaString(env, package_name));
}

// static
bool ChromeWebApkHost::IsPermissionRevokedByPolicy(
    const std::string& package_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_ChromeWebApkHost_isPermissionRevokedByPolicy(
      env, base::android::ConvertUTF8ToJavaString(env, package_name));
}

// static
jboolean CanLaunchRendererInWebApkProcess(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  return variations::GetVariationParamValueByFeature(
             chrome::android::kImprovedA2HS, kLaunchRendererInWebApkProcess) ==
         "true";
}
