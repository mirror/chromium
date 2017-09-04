// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_NEAR_OOM_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_ANDROID_NEAR_OOM_INFOBAR_DELEGATE_H_

#include "base/macros.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

namespace content {
class RenderProcessHost;
}  // namespace content

class InfoBarService;

class NearOomInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  static void Create(InfoBarService* infobar_service,
                     content::RenderProcessHost* render_process_host);

 private:
  explicit NearOomInfoBarDelegate(
      content::RenderProcessHost* render_process_host);
  ~NearOomInfoBarDelegate() override;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  base::string16 GetMessageText() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;

  content::RenderProcessHost* render_process_host_;

  DISALLOW_COPY_AND_ASSIGN(NearOomInfoBarDelegate);
};

#endif  // CHROME_BROWSER_ANDROID_NEAR_OOM_INFOBAR_DELEGATE_H_
