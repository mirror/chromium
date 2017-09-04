// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/near_oom_infobar_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar.h"
#include "content/public/browser/render_process_host.h"

// static
void NearOomInfoBarDelegate::Create(
    InfoBarService* infobar_service,
    content::RenderProcessHost* render_process_host) {
  DCHECK(render_process_host);
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new NearOomInfoBarDelegate(render_process_host))));
}

NearOomInfoBarDelegate::NearOomInfoBarDelegate(
    content::RenderProcessHost* render_process_host)
    : render_process_host_(render_process_host) {
  CHECK(render_process_host_);
}

NearOomInfoBarDelegate::~NearOomInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
NearOomInfoBarDelegate::GetIdentifier() const {
  return NEAR_OOM_INFOBAR_ANDROID;
}

base::string16 NearOomInfoBarDelegate::GetMessageText() const {
  return base::UTF8ToUTF16("Stopped JS execution");
}

base::string16 NearOomInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return button == BUTTON_OK ? base::UTF8ToUTF16("OK")
                             : base::UTF8ToUTF16("Resume execution");
}

bool NearOomInfoBarDelegate::Accept() {
  return true;
}

bool NearOomInfoBarDelegate::Cancel() {
  render_process_host_->ResumePages();
  return true;
}
