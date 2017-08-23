// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_DELEGATE_BRIDGE_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_DELEGATE_BRIDGE_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

namespace infobars {
class NotifyInfoBarDelegate;
}

// Wrapper class that exposes data from a native NotifyInfoBarDelegate through
// JNI.
class NotifyInfoBarDelegateBridge {
 public:
  explicit NotifyInfoBarDelegateBridge(
      infobars::NotifyInfoBarDelegate* delegate);
  virtual ~NotifyInfoBarDelegateBridge();

  // JNI accessors.
  base::android::ScopedJavaLocalRef<jstring> GetDescription(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetShortDescription(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  base::android::ScopedJavaLocalRef<jstring> GetFeaturedLinkText(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);
  jint GetEnumeratedIcon(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj);
  void OnLinkTapped(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj);

 private:
  // Weak pointer, the delegate is owned by the InfoBar.
  infobars::NotifyInfoBarDelegate* infobar_delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotifyInfoBarDelegateBridge);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_NOTIFY_INFOBAR_DELEGATE_BRIDGE_H_
