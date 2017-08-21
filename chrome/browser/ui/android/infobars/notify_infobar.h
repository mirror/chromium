// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/infobar_android.h"
#include "chrome/browser/ui/android/infobars/notify_infobar_delegate_android.h"

namespace infobars {
class NotifyInfoBarDelegate;
}

// A NotifyInfoBar displays some information for the user to notify them about
// something the browser did. It displays as secondary control a link that
// the user can follow to know more or be redirected somewhere.
// To define the info displayed and the behaviour of the link, implement
// infobars::NotifyInfoBarDelegate. It will have to be wrapped in a
// NotifyInfoBarDelegateAndroid that will be responsible of exposing the data
// to the Java implementation.
class NotifyInfoBar : public InfoBarAndroid {
 public:
  explicit NotifyInfoBar(
      std::unique_ptr<NotifyInfoBarDelegateAndroid> android_delegate);
  ~NotifyInfoBar() override;

 private:
  // InfoBarAndroid overrides.
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;
  void ProcessButton(int action) override;

  // Object used by the Java NotifyInfoBar to access properties and callbacks.
  // The InfoBarDelegate it wraps is owned by this class' parent, InfoBar.
  std::unique_ptr<NotifyInfoBarDelegateAndroid> android_delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotifyInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_H_
