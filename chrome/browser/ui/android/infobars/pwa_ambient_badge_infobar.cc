// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/pwa_ambient_badge_infobar.h"

#include <utility>

#include "base/android/jni_string.h"
#include "chrome/browser/installable/pwa_ambient_badge_infobar_delegate_android.h"
#include "jni/PwaAmbientBadgeInfoBar_jni.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/image/image.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

PwaAmbientBadgeInfoBar::PwaAmbientBadgeInfoBar(
    std::unique_ptr<PwaAmbientBadgeInfoBarDelegateAndroid> delegate)
    : InfoBarAndroid(std::move(delegate)) {}

PwaAmbientBadgeInfoBar::~PwaAmbientBadgeInfoBar() {}

ScopedJavaLocalRef<jobject> PwaAmbientBadgeInfoBar::CreateRenderInfoBar(
    JNIEnv* env) {
  PwaAmbientBadgeInfoBarDelegateAndroid* delegate = GetDelegate();

  base::android::ScopedJavaLocalRef<jstring> message_text =
      base::android::ConvertUTF16ToJavaString(env, delegate->GetMessageText());

  DCHECK(!delegate->GetPrimaryIcon().drawsNothing());
  base::android::ScopedJavaLocalRef<jobject> java_bitmap =
      gfx::ConvertToJavaBitmap(&delegate->GetPrimaryIcon());

  return Java_PwaAmbientBadgeInfoBar_show(env, delegate->GetIconId(),
                                          java_bitmap, message_text);
}

void PwaAmbientBadgeInfoBar::OnLinkClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (!owner())
    return;

  // TODO(dominickn): hook up add to home screen from here.
  RemoveSelf();
}

void PwaAmbientBadgeInfoBar::ProcessButton(int action) {
  if (!owner())
    return;

  RemoveSelf();
}

PwaAmbientBadgeInfoBarDelegateAndroid* PwaAmbientBadgeInfoBar::GetDelegate() {
  return static_cast<PwaAmbientBadgeInfoBarDelegateAndroid*>(delegate());
}
