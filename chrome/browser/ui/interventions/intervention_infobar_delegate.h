// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_INTERVENTIONS_INTERVENTION_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_INTERVENTIONS_INTERVENTION_INFOBAR_DELEGATE_H_

#include "base/callback.h"
#include "components/infobars/core/infobar_delegate.h"

// An specialized implementation of InfoBarDelegate used by intervention
// infobars. It runs a callback when the infobar is dismissed.
class InterventionInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  InterventionInfoBarDelegate(
      infobars::InfoBarDelegate::InfoBarIdentifier identifier,
      base::OnceClosure dismiss_callback);
  ~InterventionInfoBarDelegate() override;

  // infobars::InfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;
  void InfoBarDismissed() override;

 private:
  const infobars::InfoBarDelegate::InfoBarIdentifier identifier_;
  base::OnceClosure dismiss_callback_;
};

#endif  // CHROME_BROWSER_UI_INTERVENTIONS_INTERVENTION_INFOBAR_DELEGATE_H_
