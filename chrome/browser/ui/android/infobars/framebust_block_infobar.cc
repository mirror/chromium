// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/framebust_block_infobar.h"

#include <memory>
#include <utility>

#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "jni/FramebustBlockInfoBar_jni.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertJavaStringToUTF8;

namespace {
class FramebustBlockInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  FramebustBlockInfoBarDelegate() = default;
  ~FramebustBlockInfoBarDelegate() override = default;

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return InfoBarDelegate::InfoBarIdentifier::FRAMEBUST_BLOCK_INFOBAR_ANDROID;
  }

  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    return delegate->GetIdentifier() == GetIdentifier();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FramebustBlockInfoBarDelegate);
};
}  // namespace

FramebustBlockInfoBar::FramebustBlockInfoBar(
    std::unique_ptr<infobars::InfoBarDelegate> delegate,
    const GURL& blocked_url)
    : InfoBarAndroid(std::move(delegate)), blocked_url_(blocked_url) {}

FramebustBlockInfoBar::~FramebustBlockInfoBar() = default;

void FramebustBlockInfoBar::ProcessButton(int action) {
  NOTREACHED();
}

void FramebustBlockInfoBar::OnLinkClicked(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  if (!owner())
    return;

  // Open the blocked link. We explicitly make the call instead of going through
  // `owner()->OpenURL()` because that method does not allow opening links in
  // the current tab.
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(this);
  web_contents->OpenURL(content::OpenURLParams(
      blocked_url_, content::Referrer(), WindowOpenDisposition::CURRENT_TAB,
      ui::PAGE_TRANSITION_LINK, false));
}

ScopedJavaLocalRef<jobject> FramebustBlockInfoBar::CreateRenderInfoBar(
    JNIEnv* env) {
  return Java_FramebustBlockInfoBar_create(
      env, ConvertUTF8ToJavaString(env, blocked_url_.spec()));
}

void Create(JNIEnv* env,
            const JavaParamRef<jclass>& j_caller,
            const JavaParamRef<jobject>& j_tab,
            const JavaParamRef<jstring>& j_url) {
  InfoBarService* service = InfoBarService::FromWebContents(
      TabAndroid::GetNativeTab(env, j_tab)->web_contents());
  service->AddInfoBar(base::MakeUnique<FramebustBlockInfoBar>(
      base::MakeUnique<FramebustBlockInfoBarDelegate>(),
      GURL(ConvertJavaStringToUTF8(env, j_url))));
}
