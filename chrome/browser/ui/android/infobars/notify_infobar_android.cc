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
    std::unique_ptr<infobars::NotifyInfoBarDelegate> delegate,
    std::unique_ptr<NotifyInfoBarDelegateBridge> delegate_bridge)
    : InfoBarAndroid(std::move(delegate)),
      delegate_bridge_(std::move(delegate_bridge)) {}

NotifyInfoBarAndroid::~NotifyInfoBarAndroid() = default;

void NotifyInfoBarAndroid::ProcessButton(int action) {
  NOTREACHED();
}

ScopedJavaLocalRef<jobject> NotifyInfoBarAndroid::CreateRenderInfoBar(
    JNIEnv* env) {
  return Java_NotifyInfoBar_create(
      env, reinterpret_cast<uintptr_t>(delegate_bridge_.get()));
}

// static
void NotifyInfoBarAndroid::Show(
    content::WebContents* web_contents,
    std::unique_ptr<infobars::NotifyInfoBarDelegate> delegate) {
  InfoBarService* service = InfoBarService::FromWebContents(web_contents);
  std::unique_ptr<NotifyInfoBarDelegateBridge> delegate_bridge =
      base::MakeUnique<NotifyInfoBarDelegateBridge>(delegate.get());
  service->AddInfoBar(base::WrapUnique(new NotifyInfoBarAndroid(
      std::move(delegate), std::move(delegate_bridge))));
}
