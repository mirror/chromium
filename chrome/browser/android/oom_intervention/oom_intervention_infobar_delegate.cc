// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/oom_intervention_infobar_delegate.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/android/oom_intervention/oom_intervention_host.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar.h"

// static
void OomInterventionInfoBarDelegate::Create(InfoBarService* infobar_service,
                                            OomInterventionHost* intervention) {
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(
          new OomInterventionInfoBarDelegate(intervention))));
}

OomInterventionInfoBarDelegate::OomInterventionInfoBarDelegate(
    OomInterventionHost* intervention)
    : intervention_(intervention) {}

OomInterventionInfoBarDelegate::~OomInterventionInfoBarDelegate() = default;

infobars::InfoBarDelegate::InfoBarIdentifier
OomInterventionInfoBarDelegate::GetIdentifier() const {
  return OOM_INTERVENTION_INFOBAR_ANDROID;
}

base::string16 OomInterventionInfoBarDelegate::GetMessageText() const {
  return base::UTF8ToUTF16(
      "This page uses too much memory, so Chrome paused it.");
}

base::string16 OomInterventionInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return button == BUTTON_OK ? base::UTF8ToUTF16("OK")
                             : base::UTF8ToUTF16("Resume");
}

bool OomInterventionInfoBarDelegate::Accept() {
  intervention_->AcceptIntervention();
  return true;
}

bool OomInterventionInfoBarDelegate::Cancel() {
  intervention_->DeclineIntervention();
  return true;
}
