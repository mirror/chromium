// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/popup_block_infobar.h"

#include <memory>
#include <utility>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/ui/interventions/intervention_infobar_delegate.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/PopupBlockInfoBar_jni.h"

PopupBlockInfoBar::PopupBlockInfoBar(
    const std::vector<GURL>& blocked_urls,
    std::unique_ptr<infobars::InfoBarDelegate> delegate)
    : InfoBarAndroid(std::move(delegate)), blocked_urls_(blocked_urls) {}

PopupBlockInfoBar::~PopupBlockInfoBar() = default;

void PopupBlockInfoBar::ProcessButton(int action) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.

  DCHECK_EQ(action, InfoBarAndroid::ACTION_OK);
  RemoveSelf();
}

void PopupBlockInfoBar::OnLinkClicked(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.

  // Tapping the link means that the user wants to bypass the intervention by
  // navigating to the blocked URL.
  RemoveSelf();
}

base::android::ScopedJavaLocalRef<jobject>
PopupBlockInfoBar::CreateRenderInfoBar(JNIEnv* env) {
  std::vector<std::string> str_array;
  for (const GURL& url : blocked_urls_)
    str_array.push_back(url.spec());
  return Java_PopupBlockInfoBar_create(
      env, base::android::ToJavaArrayOfStrings(env, str_array));
}
