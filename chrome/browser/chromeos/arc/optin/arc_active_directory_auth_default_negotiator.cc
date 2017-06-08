// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/optin/arc_active_directory_auth_default_negotiator.h"

#include <string>

#include "base/memory/ptr_util.h"

namespace arc {

ArcActiveDirectoryAuthDefaultNegotiator::
    ArcActiveDirectoryAuthDefaultNegotiator(ArcSupportHost* support_host)
    : support_host_(support_host) {
  DCHECK(support_host_);
}

ArcActiveDirectoryAuthDefaultNegotiator::
    ~ArcActiveDirectoryAuthDefaultNegotiator() {
  support_host_->SetTermsOfServiceDelegate(nullptr);
}

void ArcActiveDirectoryAuthDefaultNegotiator::StartNegotiationImpl() {
  support_host_->SetTermsOfServiceDelegate(this);
  support_host_->ShowActiveDirectoryAuthNotification();
}

void ArcActiveDirectoryAuthDefaultNegotiator::OnTermsAgreed(
    bool is_metrics_enabled,
    bool is_backup_and_restore_enabled,
    bool is_location_service_enabled) {
  support_host_->SetTermsOfServiceDelegate(nullptr);
  ReportResult(true);
}

void ArcActiveDirectoryAuthDefaultNegotiator::OnTermsRejected() {
  // User cancels Active Directory auth notification UI by clicking "Cancel"
  // button or closing the window directly.
  support_host_->SetTermsOfServiceDelegate(nullptr);
  ReportResult(false);
}

void ArcActiveDirectoryAuthDefaultNegotiator::OnTermsRetryClicked() {
  support_host_->ShowActiveDirectoryAuthNotification();
}

}  // namespace arc
