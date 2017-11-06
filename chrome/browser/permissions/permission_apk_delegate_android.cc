// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/permissions/permission_apk_delegate_android.h"

#include "base/android/jni_array.h"
#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "chrome/browser/android/preferences/pref_service_bridge.h"
#include "jni/PermissionApkDelegate_jni.h"

namespace {
using DelegateMap =
    std::map<std::string, std::unique_ptr<PermissionApkDelegateAndroid>>;

base::LazyInstance<DelegateMap>::DestructorAtExit g_delegate_map =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

PermissionApkDelegateAndroid::PermissionApkDelegateAndroid(
    const std::string& package_name)
    : package_name_(package_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_package_name =
      base::android::ConvertUTF8ToJavaString(env, package_name);
  java_ref_.Reset(Java_PermissionApkDelegate_create(
      env, reinterpret_cast<intptr_t>(this), java_package_name));
}

PermissionApkDelegateAndroid::~PermissionApkDelegateAndroid() {}

// static
PermissionApkDelegateAndroid* PermissionApkDelegateAndroid::Get(
    const std::string& package_name) {
  DelegateMap* map = g_delegate_map.Pointer();
  DelegateMap::iterator it = map->find(package_name);
  if (it == map->end()) {
    map->insert(std::make_pair(
        package_name,
        std::make_unique<PermissionApkDelegateAndroid>(package_name)));
  }
  return map->find(package_name)->second.get();
}

void PermissionApkDelegateAndroid::RequestPermission(
    const std::vector<ContentSettingsType>& content_settings_types,
    const OnRequestPermissionsResultCallback& callback) {
  if (!callback_.is_null()) {
    base::ResetAndReturn(&callback_).Run(false);
    return;
  }

  callback_ = callback;
  std::vector<std::string> android_permissions;
  for (ContentSettingsType content_settings_type : content_settings_types) {
    PrefServiceBridge::GetAndroidPermissionsForContentSetting(
        content_settings_type, &android_permissions);
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PermissionApkDelegate_requestPermission(
      env, java_ref_,
      base::android::ToJavaArrayOfStrings(env, android_permissions));
}

void PermissionApkDelegateAndroid::OnRequestPermissionsResult(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jboolean all_permissions_granted) {
  base::ResetAndReturn(&callback_).Run(all_permissions_granted);
}
