// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/offline_pages/downloads/offline_page_download_manager_bridge.h"

#include "chrome/browser/offline_pages/android/offline_page_bridge.h"
#include "jni/OfflinePageDownloadManagerBridge_jni.h"

namespace offline_pages {
namespace android {

using AddPageCallback = OfflinePageDownloadManagerBridge::AddPageCallback;

void RunAddPageCallback(JNIEnv* env,
                        const base::android::JavaParamRef<jclass>& clazz,
                        jlong j_callback_pointer,
                        jlong j_offline_id,
                        jlong j_system_download_id) {
  DCHECK(j_callback_pointer);
  AddPageCallback* add_page_callback =
      reinterpret_cast<AddPageCallback*>(j_callback_pointer);
  std::move(*add_page_callback).Run(j_offline_id, j_system_download_id);
}

// static
void OfflinePageDownloadManagerBridge::AddPageToDownloadManager(
    const OfflinePageItem& offline_page,
    AddPageCallback callback) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> j_offline_page =
      OfflinePageBridge::ConvertToJavaOfflinePage(env, offline_page);

  uintptr_t callback_pointer =
      reinterpret_cast<uintptr_t>(new AddPageCallback(std::move(callback)));

  Java_OfflinePageDownloadManagerBridge_addPage(env, j_offline_page,
                                                callback_pointer);
}

}  // namespace android
}  // namespace offline_pages
