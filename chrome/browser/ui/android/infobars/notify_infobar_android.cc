// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/notify_infobar_android.h"

#include <memory>
#include <utility>

#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/android/infobars/notify_infobar_delegate_bridge.h"
#include "components/infobars/core/notify_infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/NotifyInfoBar_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertJavaStringToUTF8;

NotifyInfoBarAndroid::NotifyInfoBarAndroid(
    std::unique_ptr<NotifyInfoBarDelegateBridge> android_delegate)
    : InfoBarAndroid(base::WrapUnique<infobars::NotifyInfoBarDelegate>(
          android_delegate->GetInfoBarDelegate())),
      android_delegate_(std::move(android_delegate)) {}

NotifyInfoBarAndroid::~NotifyInfoBarAndroid() = default;

void NotifyInfoBarAndroid::ProcessButton(int action) {
  NOTREACHED();
}

ScopedJavaLocalRef<jobject> NotifyInfoBarAndroid::CreateRenderInfoBar(
    JNIEnv* env) {
  return Java_NotifyInfoBar_create(
      env, reinterpret_cast<uintptr_t>(android_delegate_.get()));
}

void Create(JNIEnv* env,
            const JavaParamRef<jclass>& j_caller,
            const JavaParamRef<jobject>& j_tab,
            jlong native_delegate) {
  NotifyInfoBarDelegateBridge* raw_delegate =
      reinterpret_cast<NotifyInfoBarDelegateBridge*>(native_delegate);
  InfoBarService* service = InfoBarService::FromWebContents(
      TabAndroid::GetNativeTab(env, j_tab)->web_contents());
  service->AddInfoBar(base::MakeUnique<NotifyInfoBarAndroid>(
      base::WrapUnique<NotifyInfoBarDelegateBridge>(raw_delegate)));
}
