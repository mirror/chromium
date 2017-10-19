// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/near_oom_infobar.h"

#include <memory>
#include <utility>

#include "base/android/jni_string.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar_delegate.h"
#include "content/public/browser/web_contents.h"
#include "jni/NearOomInfoBar_jni.h"

namespace {

class NearOomInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return infobars::InfoBarDelegate::InfoBarIdentifier::
        NEAR_OOM_INFOBAR_ANDROID;
  }

  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    return delegate->GetIdentifier() == GetIdentifier();
  }
};

}  // namespace

NearOomInfoBar::NearOomInfoBar()
    : InfoBarAndroid(base::MakeUnique<NearOomInfoBarDelegate>()) {}

NearOomInfoBar::~NearOomInfoBar() = default;

void NearOomInfoBar::ProcessButton(int action) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.

  DCHECK((action == InfoBarAndroid::ACTION_OK) ||
         (action == InfoBarAndroid::ACTION_CANCEL));

  // TODO(bashi): Implement
  if (action == InfoBarAndroid::ACTION_OK) {
    DLOG(DEBUG) << "Near-OOM Intervention accepted.";
    // delegate->AcceptIntervention();
  } else {
    DLOG(DEBUG) << "Near-OOM Intervention declined.";
    // delegate->DeclineIntervention();
  }

  RemoveSelf();
}

base::android::ScopedJavaLocalRef<jobject> NearOomInfoBar::CreateRenderInfoBar(
    JNIEnv* env) {
  return Java_NearOomInfoBar_create(env);
}

// static
void NearOomInfoBar::Show(content::WebContents* web_contents) {
  InfoBarService* service = InfoBarService::FromWebContents(web_contents);
  service->AddInfoBar(base::WrapUnique(new NearOomInfoBar()));
}
