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

class FramebustBlockInfoBarDelegate;

class FramebustBlockInfoBar : public InfoBarAndroid {
 public:
  explicit FramebustBlockInfoBar(
      std::unique_ptr<FramebustBlockInfoBarDelegate> delegate);
  ~FramebustBlockInfoBar() override;
  base::android::ScopedJavaLocalRef<jstring> GetDescription(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetShortDescription(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetLink(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

 protected:
  // InfoBarAndroid overrides.
  void ProcessButton(int action) override;
  void OnLinkClicked(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj) override;
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;

 private:
  FramebustBlockInfoBarDelegate* GetDelegate();
  DISALLOW_COPY_AND_ASSIGN(FramebustBlockInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_
