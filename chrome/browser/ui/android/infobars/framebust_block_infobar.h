// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/infobar_android.h"
#include "components/infobars/core/infobar_delegate.h"
#include "url/gurl.h"

class FramebustBlockInfoBar : public InfoBarAndroid {
 public:
  FramebustBlockInfoBar(std::unique_ptr<infobars::InfoBarDelegate> delegate,
                        const GURL& blocked_url);
  ~FramebustBlockInfoBar() override;

 protected:
  // InfoBarAndroid overrides.
  void ProcessButton(int action) override;
  void OnLinkClicked(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj) override;
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;

 private:
  const GURL blocked_url_;

  DISALLOW_COPY_AND_ASSIGN(FramebustBlockInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_
