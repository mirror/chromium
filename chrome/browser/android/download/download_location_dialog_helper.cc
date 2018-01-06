// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/download_location_dialog_helper.h"

#include "base/android/jni_string.h"
#include "content/public/browser/web_contents.h"
#include "jni/DownloadLocationDialogHelper_jni.h"

DownloadLocationDialogHelper::DownloadLocationDialogHelper() {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_obj_.Reset(env, Java_DownloadLocationDialogHelper_create(
                           env, reinterpret_cast<long>(this))
                           .obj());
  DCHECK(!java_obj_.is_null());
}

DownloadLocationDialogHelper::~DownloadLocationDialogHelper() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DownloadLocationDialogHelper_destroy(env, java_obj_);
}

void DownloadLocationDialogHelper::ShowDialog(
    content::WebContents* web_contents,
    base::FilePath suggested_path,
    const DownloadTargetDeterminerDelegate::ConfirmationCallback& callback) {
  dialog_complete_callback_ = callback;

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DownloadLocationDialogHelper_showDialog(
      env, java_obj_, web_contents->GetJavaWebContents(),
      base::android::ConvertUTF8ToJavaString(env,
                                             suggested_path.AsUTF8Unsafe()));
}

void DownloadLocationDialogHelper::OnComplete(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& returned_path) {
  std::string path_string(
      base::android::ConvertJavaStringToUTF8(env, returned_path));

  if (dialog_complete_callback_) {
    dialog_complete_callback_.Run(
        DownloadConfirmationResult::CONTINUE_WITHOUT_CONFIRMATION,
        base::FilePath(FILE_PATH_LITERAL(path_string)));
  }
}
