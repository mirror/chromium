// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_BRIDGE_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "chrome/browser/download/download_confirmation_result.h"
#include "chrome/browser/download/download_target_determiner_delegate.h"
#include "content/public/browser/download_item.h"
#include "ui/gfx/native_widget_types.h"

class DownloadLocationDialogBridge {
 public:
  DownloadLocationDialogBridge();
  ~DownloadLocationDialogBridge();

  void ShowDialog(
      content::DownloadItem* download,
      const base::FilePath& suggested_path,
      const DownloadTargetDeterminerDelegate::ConfirmationCallback& callback);

  void OnComplete(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj,
                  const base::android::JavaParamRef<jstring>& returned_path);

  void OnCanceled(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  jboolean is_dialog_showing_;
  base::android::ScopedJavaGlobalRef<jobject> java_obj_;
  DownloadTargetDeterminerDelegate::ConfirmationCallback
      dialog_complete_callback_;
  content::DownloadItem* download_;

  DISALLOW_COPY_AND_ASSIGN(DownloadLocationDialogBridge);
};

#endif  // CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_BRIDGE_H_
