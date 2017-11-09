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
#include "chrome/browser/ui/android/interventions/framebust_block_message_delegate_bridge.h"
#include "chrome/browser/ui/interventions/framebust_block_message_delegate.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/FramebustBlockInfoBar_jni.h"

typedef infobars::InfoBarDelegate::InfoBarIdentifier InfoBarIdentifier;

namespace {
class FramebustBlockInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  explicit FramebustBlockInfoBarDelegate(const GURL& blocked_url)
      : infobars::InfoBarDelegate(), blocked_url_(blocked_url) {}

  InfoBarIdentifier GetIdentifier() const override {
    return InfoBarIdentifier::FRAMEBUST_BLOCK_INFOBAR_ANDROID;
  }

  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    if (delegate->GetIdentifier() != GetIdentifier())
      return false;

    auto* framebust_delegate =
        static_cast<FramebustBlockInfoBarDelegate*>(delegate);
    return blocked_url_ == framebust_delegate->blocked_url_;
  }

 private:
  const GURL& blocked_url_;
};

}  // namespace

FramebustBlockInfoBar::FramebustBlockInfoBar(
    std::unique_ptr<FramebustBlockMessageDelegate> delegate)
    : InfoBarAndroid(base::MakeUnique<FramebustBlockInfoBarDelegate>(
          delegate->GetBlockedUrl())),
      delegate_bridge_(base::MakeUnique<FramebustBlockMessageDelegateBridge>(
          std::move(delegate))) {}

FramebustBlockInfoBar::~FramebustBlockInfoBar() = default;

void FramebustBlockInfoBar::ProcessButton(int action) {
  // Closes the infobar from the Java UI code.
  NOTREACHED();
}

base::android::ScopedJavaLocalRef<jobject>
FramebustBlockInfoBar::CreateRenderInfoBar(JNIEnv* env) {
  return Java_FramebustBlockInfoBar_create(
      env, reinterpret_cast<uintptr_t>(delegate_bridge_.get()));
}

// static
void FramebustBlockInfoBar::Show(
    content::WebContents* web_contents,
    std::unique_ptr<FramebustBlockMessageDelegate> message_delegate) {
  InfoBarService* service = InfoBarService::FromWebContents(web_contents);
  auto existing_infobars = service->GetInfoBarsForId(
      InfoBarIdentifier::FRAMEBUST_BLOCK_INFOBAR_ANDROID);

  // We close the previous infobar before showing the new one, so we should
  // never have more than one.
  DCHECK_LE(existing_infobars.size(), 1u);

  std::unique_ptr<InfoBar> new_infobar =
      base::WrapUnique(new FramebustBlockInfoBar(std::move(message_delegate)));
  if (existing_infobars.empty()) {
    service->AddInfoBar(std::move(new_infobar));
  } else {
    service->ReplaceInfoBar(existing_infobars.front(), std::move(new_infobar));
  }
}
