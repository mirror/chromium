// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/mirror_account_reconcilor_delegate.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "components/signin/core/browser/account_reconcilor.h"
#include "components/signin/core/browser/reconcilor_delegate_helper.h"
#include "google_apis/gaia/google_service_auth_error.h"

namespace {

#if defined(OS_CHROMEOS)
constexpr base::TimeDelta kAccountReconcilorTimeout =
    base::TimeDelta::FromSeconds(10);
#endif

}  // namespace

namespace signin {

MirrorAccountReconcilorDelegate::MirrorAccountReconcilorDelegate(
    SigninManagerBase* signin_manager,
    std::unique_ptr<ReconcilorDelegateHelper> helper,
    bool is_child_account)
    : signin_manager_(signin_manager),
      helper_(std::move(helper)),
      is_child_account_(is_child_account) {
  DCHECK(signin_manager_);
  signin_manager_->AddObserver(this);
}

MirrorAccountReconcilorDelegate::~MirrorAccountReconcilorDelegate() {
  signin_manager_->RemoveObserver(this);
}

bool MirrorAccountReconcilorDelegate::IsReconcileEnabled() const {
  return signin_manager_->IsAuthenticated();
}

bool MirrorAccountReconcilorDelegate::IsAccountConsistencyEnforced() const {
  return true;
}

bool MirrorAccountReconcilorDelegate::ShouldAbortReconcileIfPrimaryHasError()
    const {
  return true;
}

std::string MirrorAccountReconcilorDelegate::GetFirstGaiaAccountForReconcile(
    const std::vector<std::string>& chrome_accounts,
    const std::vector<gaia::ListedAccount>& gaia_accounts,
    const std::string& primary_account,
    bool first_execution) const {
  // Mirror only uses the primary account, and it is never empty.
  DCHECK(!primary_account.empty());
  DCHECK(base::ContainsValue(chrome_accounts, primary_account));
  return primary_account;
}

void MirrorAccountReconcilorDelegate::GoogleSigninSucceeded(
    const std::string& account_id,
    const std::string& username) {
  reconcilor()->EnableReconcile();
}

void MirrorAccountReconcilorDelegate::GoogleSignedOut(
    const std::string& account_id,
    const std::string& username) {
  reconcilor()->DisableReconcile(true /* logout_all_gaia_accounts */);
}

base::TimeDelta MirrorAccountReconcilorDelegate::GetReconcileTimeout() const {
#if defined(OS_CHROMEOS)
  if (is_child_account_)
    return kAccountReconcilorTimeout;
#endif

  // Do not set a reconciliation timeout.
  return base::TimeDelta::Max();
}

void MirrorAccountReconcilorDelegate::OnReconcileError(
    const GoogleServiceAuthError& error) {
#if !defined(OS_CHROMEOS)
  NOTREACHED();
#endif
  // As of now, this method should only be called for Unicorn users on ChromeOS.
  DCHECK(is_child_account_);

  if (error.state() == GoogleServiceAuthError::CONNECTION_FAILED) {
    helper_->ForceUserOnlineSignIn();
    helper_->AttemptUserExit();
  }
}

}  // namespace signin
