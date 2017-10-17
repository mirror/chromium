// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_INFOBAR_DELEGATE_H_

#include "base/macros.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

class InfoBarService;
class OomInterventionHost;

// TODO(???): This infobar delegate doesn't match the current UI mock.
class OomInterventionInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  static void Create(InfoBarService* infobar_service,
                     OomInterventionHost* intervention);

 private:
  OomInterventionInfoBarDelegate(OomInterventionHost* intervention);
  ~OomInterventionInfoBarDelegate() override;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  base::string16 GetMessageText() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;

  OomInterventionHost* intervention_;

  DISALLOW_COPY_AND_ASSIGN(OomInterventionInfoBarDelegate);
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_INFOBAR_DELEGATE_H_
