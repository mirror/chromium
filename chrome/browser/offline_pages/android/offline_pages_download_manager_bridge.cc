// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/android/offline_pages_download_manager_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "jni/OfflinePagesDownloadManagerBridge_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace offline_pages {
namespace android {

// Returns true if ADM is available on this phone.
bool OfflinePagesDownloadManagerBridge::IsAndroidDownloadManagerInstalled() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jboolean is_installed =
      Java_OfflinePagesDownloadManagerBridge_isAndroidDownloadManagerInstalled(
          env);
  return is_installed;
}

// Returns the ADM ID of the download, which we will place in the offline
// pages database as part of the offline page item.
long OfflinePagesDownloadManagerBridge::addCompletedDownload(
    std::string title,
    std::string description,
    bool is_media_scanner_scannable,
    std::string mime_type,
    std::string path,
    long length,
    bool show_notification,
    std::string uri,
    std::string referer) {
  JNIEnv* env = base::android::AttachCurrentThread();
  // Convert strings to jstring references.
  ScopedJavaLocalRef<jstring> j_title =
      base::android::ConvertUTF8ToJavaString(env, title);
  ScopedJavaLocalRef<jstring> j_description =
      base::android::ConvertUTF8ToJavaString(env, description);
  ScopedJavaLocalRef<jstring> j_mime_type =
      base::android::ConvertUTF8ToJavaString(env, mime_type);
  ScopedJavaLocalRef<jstring> j_path =
      base::android::ConvertUTF8ToJavaString(env, path);
  ScopedJavaLocalRef<jstring> j_uri =
      base::android::ConvertUTF8ToJavaString(env, uri);
  ScopedJavaLocalRef<jstring> j_referer =
      base::android::ConvertUTF8ToJavaString(env, referer);

  long count = Java_OfflinePagesDownloadManagerBridge_addCompletedDownload(
      env, j_title, j_description, is_media_scanner_scannable, j_mime_type,
      j_path, length, show_notification, j_uri, j_referer);
  return count;
}

// Returns the number of pages removed.
long OfflinePagesDownloadManagerBridge::remove(
    std::vector<int64_t>& android_download_manager_ids) {
  JNIEnv* env = base::android::AttachCurrentThread();
  // Build a JNI array with our ID data.
  ScopedJavaLocalRef<jlongArray> j_ids =
      base::android::ToJavaLongArray(env, android_download_manager_ids);

  long download_id = Java_OfflinePagesDownloadManagerBridge_remove(env, j_ids);

  return download_id;
}

}  // namespace android
}  // namespace offline_pages
