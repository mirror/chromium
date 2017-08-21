// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/notify_infobar_delegate_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/android/resource_mapper.h"
#include "components/infobars/core/notify_infobar_delegate.h"
#include "jni/NotifyInfoBarDelegate_jni.h"

using base::android::ScopedJavaLocalRef;
using base::android::JavaParamRef;
using base::android::ConvertUTF16ToJavaString;

NotifyInfoBarDelegateAndroid::NotifyInfoBarDelegateAndroid(
    infobars::NotifyInfoBarDelegate* delegate)
    : infobar_delegate_(delegate) {}

NotifyInfoBarDelegateAndroid::~NotifyInfoBarDelegateAndroid() = default;

infobars::NotifyInfoBarDelegate*
NotifyInfoBarDelegateAndroid::GetInfoBarDelegate() const {
  return infobar_delegate_;
}

ScopedJavaLocalRef<jstring> NotifyInfoBarDelegateAndroid::GetDescription(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF16ToJavaString(env, infobar_delegate_->GetDescription());
}
ScopedJavaLocalRef<jstring> NotifyInfoBarDelegateAndroid::GetShortDescription(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF16ToJavaString(env,
                                  infobar_delegate_->GetShortDescription());
}
ScopedJavaLocalRef<jstring> NotifyInfoBarDelegateAndroid::GetFeaturedLinkText(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF16ToJavaString(env,
                                  infobar_delegate_->GetFeaturedLinkText());
}
jint NotifyInfoBarDelegateAndroid::GetEnumeratedIcon(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ResourceMapper::MapFromChromiumId(infobar_delegate_->GetIconId());
}
void NotifyInfoBarDelegateAndroid::OnLinkTapped(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  infobar_delegate_->OnLinkTapped();
}
