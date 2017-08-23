// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/notify_infobar_delegate_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/android/resource_mapper.h"
#include "components/infobars/core/notify_infobar_delegate.h"
#include "jni/NotifyInfoBarDelegateBridge_jni.h"

using base::android::ScopedJavaLocalRef;
using base::android::JavaParamRef;
using base::android::ConvertUTF16ToJavaString;

NotifyInfoBarDelegateBridge::NotifyInfoBarDelegateBridge(
    infobars::NotifyInfoBarDelegate* delegate)
    : infobar_delegate_(delegate) {}

NotifyInfoBarDelegateBridge::~NotifyInfoBarDelegateBridge() = default;

ScopedJavaLocalRef<jstring> NotifyInfoBarDelegateBridge::GetDescription(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF16ToJavaString(env, infobar_delegate_->GetDescription());
}

ScopedJavaLocalRef<jstring> NotifyInfoBarDelegateBridge::GetShortDescription(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF16ToJavaString(env,
                                  infobar_delegate_->GetShortDescription());
}

ScopedJavaLocalRef<jstring> NotifyInfoBarDelegateBridge::GetFeaturedLinkText(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF16ToJavaString(env,
                                  infobar_delegate_->GetFeaturedLinkText());
}

jint NotifyInfoBarDelegateBridge::GetEnumeratedIcon(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ResourceMapper::MapFromChromiumId(infobar_delegate_->GetIconId());
}

void NotifyInfoBarDelegateBridge::OnLinkTapped(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  infobar_delegate_->OnLinkTapped();
}
