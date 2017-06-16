// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_READER_MODE_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_READER_MODE_INFOBAR_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/infobar_android.h"
#include "components/infobars/core/infobar_delegate.h"

class ReaderModeInfoBarDelegate;

namespace content {
class WebContents;
}  // namespace content

class ReaderModeInfoBar : public InfoBarAndroid {
 public:
  explicit ReaderModeInfoBar(
      std::unique_ptr<ReaderModeInfoBarDelegate> delegate,
      content::WebContents* web_contents);
  ~ReaderModeInfoBar() override;

  base::android::ScopedJavaLocalRef<jobject> GetTab(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

 protected:
  infobars::InfoBarDelegate* GetDelegate();

  // InfoBarAndroid overrides.
  void ProcessButton(int action) override;
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;

 private:
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(ReaderModeInfoBar);
};

// Register native methods.
bool RegisterReaderModeInfoBar(JNIEnv* env);

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_READER_MODE_INFOBAR_H_
