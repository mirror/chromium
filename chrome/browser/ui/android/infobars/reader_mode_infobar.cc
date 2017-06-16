// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/reader_mode_infobar.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/ReaderModeInfoBar_jni.h"
#include "ui/android/window_android.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

class ReaderModeInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  ~ReaderModeInfoBarDelegate() override {}

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return InfoBarDelegate::InfoBarIdentifier::READER_MODE_INFOBAR_ANDROID;
  }

  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    return delegate->GetIdentifier() == GetIdentifier();
  }
};

ReaderModeInfoBar::ReaderModeInfoBar(
    std::unique_ptr<ReaderModeInfoBarDelegate> delegate,
    content::WebContents* web_contents)
    : InfoBarAndroid(std::move(delegate)), web_contents_(web_contents) {}

ReaderModeInfoBar::~ReaderModeInfoBar() {}

infobars::InfoBarDelegate* ReaderModeInfoBar::GetDelegate() {
  return delegate();
}

ScopedJavaLocalRef<jobject> ReaderModeInfoBar::CreateRenderInfoBar(
    JNIEnv* env) {
  return Java_ReaderModeInfoBar_create(env);
}

base::android::ScopedJavaLocalRef<jobject> ReaderModeInfoBar::GetTab(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  DCHECK(web_contents_);

  return TabAndroid::FromWebContents(web_contents_)->GetJavaObject();
}

void ReaderModeInfoBar::ProcessButton(int action) {}

void Create(JNIEnv* env,
            const JavaParamRef<jclass>& j_caller,
            const JavaParamRef<jobject>& j_tab) {
  content::WebContents* web_contents =
      TabAndroid::GetNativeTab(env, j_tab)->web_contents();
  InfoBarService* service = InfoBarService::FromWebContents(web_contents);

  service->AddInfoBar(base::MakeUnique<ReaderModeInfoBar>(
      base::MakeUnique<ReaderModeInfoBarDelegate>(), web_contents));
}

bool RegisterReaderModeInfoBar(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
