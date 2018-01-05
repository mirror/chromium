// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_HELPER_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_HELPER_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "ui/gfx/native_widget_types.h"

namespace content {
class WebContents;
}

class DownloadLocationDialogHelper {
 public:
  DownloadLocationDialogHelper();
  ~DownloadLocationDialogHelper();

  void ShowDialog(content::WebContents* web_contents);

  void OnComplete(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadLocationDialogHelper);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_LOCATION_DIALOG_HELPER_H_
