// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_DELEGATE_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

namespace infobars {
class NotifyInfoBarDelegate;
}

// Wrapper class that exposes data from a native NotifyInfoBarDelegate through
// JNI.
class NotifyInfoBarDelegateAndroid {
 public:
  explicit NotifyInfoBarDelegateAndroid(
      infobars::NotifyInfoBarDelegate* delegate);
  virtual ~NotifyInfoBarDelegateAndroid();

  // The wrapped InfoBarDelegate, that will be used to build the native InfoBar.
  infobars::NotifyInfoBarDelegate* GetInfoBarDelegate() const;

  // JNI accessors.
  virtual base::android::ScopedJavaLocalRef<jstring> GetDescription(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  virtual base::android::ScopedJavaLocalRef<jstring> GetShortDescription(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  virtual base::android::ScopedJavaLocalRef<jstring> GetFeaturedLinkText(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  virtual jint GetEnumeratedIcon(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  virtual void OnLinkTapped(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);

 private:
  // Weak pointer, the delegate will be owned by the InfoBar once it is built.
  // Until that happens, this object will be transferring carrying the object
  // through JNI and back.
  infobars::NotifyInfoBarDelegate* infobar_delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotifyInfoBarDelegateAndroid);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_DELEGATE_ANDROID_H_
