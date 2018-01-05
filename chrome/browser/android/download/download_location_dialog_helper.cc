// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/download_location_dialog_helper.h"

#include "content/public/browser/web_contents.h"

DownloadLocationDialogHelper::DownloadLocationDialogHelper() {}

DownloadLocationDialogHelper::~DownloadLocationDialogHelper() {}

void DownloadLocationDialogHelper::ShowDialog(
    content::WebContents* web_contents) {
  LOG(ERROR) << "joy: ShowDownloadLocationDialog";
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DownloadLocationDialogHelper_showDialog(
      env, web_contents->GetJavaWebContents());
}

void DownloadLocationDialogHelper::OnComplete(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  LOG(ERROR) << "joy: OnDownloadLocationDialogComplete";
}
