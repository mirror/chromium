// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_PWA_AMBIENT_BADGE_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_PWA_AMBIENT_BADGE_INFOBAR_H_

#include <memory>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/infobar_android.h"

class PwaAmbientBadgeInfoBarDelegateAndroid;

class PwaAmbientBadgeInfoBar : public InfoBarAndroid {
 public:
  explicit PwaAmbientBadgeInfoBar(
      std::unique_ptr<PwaAmbientBadgeInfoBarDelegateAndroid> delegate);
  ~PwaAmbientBadgeInfoBar() override;

 private:
  PwaAmbientBadgeInfoBarDelegateAndroid* GetDelegate();

  // InfoBarAndroid:
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;
  void OnLinkClicked(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj) override;
  void ProcessButton(int action) override;

  DISALLOW_COPY_AND_ASSIGN(PwaAmbientBadgeInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_PWA_AMBIENT_BADGE_INFOBAR_H_
