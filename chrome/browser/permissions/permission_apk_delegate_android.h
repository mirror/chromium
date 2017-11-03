// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_PERMISSION_APK_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_PERMISSIONS_PERMISSION_APK_DELEGATE_ANDROID_H_

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "chrome/browser/permissions/permission_context_base.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

class PermissionApkDelegateAndroid {
 public:
  using OnRequestPermissionsResultCallback = base::Callback<void(bool)>;

  explicit PermissionApkDelegateAndroid(const std::string& package_name);
  static PermissionApkDelegateAndroid* Get(const std::string&);

  void RequestPermission(
      const std::vector<ContentSettingsType>& content_settings_types,
      const OnRequestPermissionsResultCallback& callback);

  void OnRequestPermissionsResult(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean all_permissions_granted);

  ~PermissionApkDelegateAndroid();

 private:
  std::string package_name_;
  OnRequestPermissionsResultCallback callback_;
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  DISALLOW_COPY_AND_ASSIGN(PermissionApkDelegateAndroid);
};

#endif  // CHROME_BROWSER_PERMISSIONS_PERMISSION_APK_DELEGATE_ANDROID_H_
