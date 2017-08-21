// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/notify_infobar.h"

#include <memory>
#include <utility>

#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/android/infobars/notify_infobar_delegate_android.h"
#include "components/infobars/core/notify_infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/NotifyInfoBar_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertJavaStringToUTF8;

NotifyInfoBar::NotifyInfoBar(
    std::unique_ptr<NotifyInfoBarDelegateAndroid> android_delegate)
    : InfoBarAndroid(base::WrapUnique<infobars::NotifyInfoBarDelegate>(
          android_delegate->GetInfoBarDelegate())),
      android_delegate_(std::move(android_delegate)) {}

NotifyInfoBar::~NotifyInfoBar() = default;

void NotifyInfoBar::ProcessButton(int action) {
  NOTREACHED();
}

ScopedJavaLocalRef<jobject> NotifyInfoBar::CreateRenderInfoBar(JNIEnv* env) {
  return Java_NotifyInfoBar_create(
      env, reinterpret_cast<uintptr_t>(android_delegate_.get()));
}

void Create(JNIEnv* env,
            const JavaParamRef<jclass>& j_caller,
            const JavaParamRef<jobject>& j_tab,
            jlong native_delegate) {
  NotifyInfoBarDelegateAndroid* raw_delegate =
      reinterpret_cast<NotifyInfoBarDelegateAndroid*>(native_delegate);
  InfoBarService* service = InfoBarService::FromWebContents(
      TabAndroid::GetNativeTab(env, j_tab)->web_contents());
  service->AddInfoBar(base::MakeUnique<NotifyInfoBar>(
      base::WrapUnique<NotifyInfoBarDelegateAndroid>(raw_delegate)));
}
