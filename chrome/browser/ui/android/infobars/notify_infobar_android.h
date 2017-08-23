// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_ANDROID_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/infobar_android.h"
#include "chrome/browser/ui/android/infobars/notify_infobar_delegate_bridge.h"

namespace infobars {
class NotifyInfoBarDelegate;
}

namespace content {
class WebContents;
}

// A NotifyInfoBarAndroid displays some information for the user to notify them
// about something the browser did. It displays as secondary control a link that
// the user can follow to know more or be redirected somewhere.
// To define the info displayed and the behaviour of the link, implement
// infobars::NotifyInfoBarDelegate.
class NotifyInfoBarAndroid : public InfoBarAndroid {
 public:
  ~NotifyInfoBarAndroid() override;

  static void Show(content::WebContents* web_contents,
                   std::unique_ptr<infobars::NotifyInfoBarDelegate> delegate);

 private:
  explicit NotifyInfoBarAndroid(
      std::unique_ptr<infobars::NotifyInfoBarDelegate> delegate,
      std::unique_ptr<NotifyInfoBarDelegateBridge> delegate_bridge);

  // InfoBarAndroid overrides.
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;
  void ProcessButton(int action) override;

  // Object used by the Java NotifyInfoBarAndroid to access properties and
  // callbacks. The InfoBarDelegate it wraps is owned by this class' parent,
  // InfoBar.
  std::unique_ptr<NotifyInfoBarDelegateBridge> delegate_bridge_;

  DISALLOW_COPY_AND_ASSIGN(NotifyInfoBarAndroid);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_ANDROID_H_
