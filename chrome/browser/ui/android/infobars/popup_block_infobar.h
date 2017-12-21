// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_

#include <vector>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/infobar_android.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

class PopupBlockedInfoBarDelegate;

// Communicates to the user about the intervention performed by the browser by
// blocking a popup.
// That InfoBar shows a link to the URL that was blocked if the user wants to
// bypass the intervention, and a "OK" button to acknowledge and accept it.
// See PopupBlockInfoBar.java for UI specifics.
class PopupBlockInfoBar : public InfoBarAndroid {
 public:
  ~PopupBlockInfoBar() override;
  explicit PopupBlockInfoBar(
      const std::vector<GURL>& blocked_urls,
      std::unique_ptr<infobars::InfoBarDelegate> delegate);

 private:
  // InfoBarAndroid:
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;
  void ProcessButton(int action) override;
  void OnLinkClicked(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj) override;

  std::vector<GURL> blocked_urls_;

  DISALLOW_COPY_AND_ASSIGN(PopupBlockInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_FRAMEBUST_BLOCK_INFOBAR_H_

