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
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/FramebustBlockInfoBar_jni.h"
#include "ui/android/window_android.h"
#include "ui/base/l10n/l10n_util.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertUTF8ToJavaString;
using base::android::ConvertUTF16ToJavaString;

namespace {
ScopedJavaLocalRef<jstring> GetJavaString(JNIEnv* env, int resource_id) {
  return ConvertUTF16ToJavaString(env, l10n_util::GetStringUTF16(resource_id));
}
}  // namespace

class FramebustBlockInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  explicit FramebustBlockInfoBarDelegate(const GURL& url) : url_(url) {}
  ~FramebustBlockInfoBarDelegate() override {}

  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return InfoBarDelegate::InfoBarIdentifier::FRAMEBUST_BLOCK_INFOBAR_ANDROID;
  }

  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    return delegate->GetIdentifier() == GetIdentifier();
  }

  void OnLinkTapped() {
    infobar()->owner()->OpenURL(url_, WindowOpenDisposition::CURRENT_TAB);
  }

  const GURL& url() const { return url_; }

  int description() const { return IDS_REDIRECT_BLOCKED_INFOBAR_DESCRIPTION; }

  int short_description() const {
    return IDS_REDIRECT_BLOCKED_INFOBAR_SHORT_DESCRIPTION;
  }

  const GURL url_;
};

FramebustBlockInfoBar::FramebustBlockInfoBar(
    std::unique_ptr<FramebustBlockInfoBarDelegate> delegate)
    : InfoBarAndroid(std::move(delegate)) {}

FramebustBlockInfoBar::~FramebustBlockInfoBar() {}

FramebustBlockInfoBarDelegate* FramebustBlockInfoBar::GetDelegate() {
  return static_cast<FramebustBlockInfoBarDelegate*>(delegate());
}

ScopedJavaLocalRef<jobject> FramebustBlockInfoBar::CreateRenderInfoBar(
    JNIEnv* env) {
  return Java_FramebustBlockInfoBar_create(env,
                                           reinterpret_cast<intptr_t>(this));
}

void FramebustBlockInfoBar::ProcessButton(int action) {
  NOTREACHED();
  RemoveSelf();
}

void FramebustBlockInfoBar::OnLinkClicked(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.

  GetDelegate()->OnLinkTapped();
}

ScopedJavaLocalRef<jstring> FramebustBlockInfoBar::GetDescription(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return GetJavaString(env, GetDelegate()->description());
}

ScopedJavaLocalRef<jstring> FramebustBlockInfoBar::GetShortDescription(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return GetJavaString(env, GetDelegate()->short_description());
}

ScopedJavaLocalRef<jstring> FramebustBlockInfoBar::GetLink(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return ConvertUTF8ToJavaString(env, GetDelegate()->url().spec());
}

void Create(JNIEnv* env,
            const JavaParamRef<jclass>& j_caller,
            const JavaParamRef<jobject>& j_tab,
            const JavaParamRef<jstring>& j_url) {
  InfoBarService* service = InfoBarService::FromWebContents(
      TabAndroid::GetNativeTab(env, j_tab)->web_contents());
  service->AddInfoBar(base::MakeUnique<FramebustBlockInfoBar>(
      base::MakeUnique<FramebustBlockInfoBarDelegate>(
          GURL(ConvertJavaStringToUTF8(env, j_url)))));
}
