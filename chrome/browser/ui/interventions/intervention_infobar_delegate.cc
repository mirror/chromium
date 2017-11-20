// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/interventions/intervention_infobar_delegate.h"

InterventionInfoBarDelegate::InterventionInfoBarDelegate(
    infobars::InfoBarDelegate::InfoBarIdentifier identifier,
    base::OnceClosure dismiss_callback)
    : identifier_(identifier), dismiss_callback_(std::move(dismiss_callback)) {}

InterventionInfoBarDelegate::~InterventionInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
InterventionInfoBarDelegate::GetIdentifier() const {
  return identifier_;
}

bool InterventionInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  return delegate->GetIdentifier() == GetIdentifier();
}

void InterventionInfoBarDelegate::InfoBarDismissed() {
  std::move(dismiss_callback_).Run();
}
