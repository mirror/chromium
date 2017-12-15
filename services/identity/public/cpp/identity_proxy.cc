// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/identity_proxy.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace identity {

IdentityProxy::IdentityProxy(SigninManagerBase* signin_manager)
    : signin_manager_(signin_manager) {
#if defined(OS_CHROMEOS)
  // On ChromeOS, the authenticated account is set very early in startup and
  // never changed. No client should be accessing the IdentityProxy before the
  // authenticated account is set.
  DCHECK(signin_manager_->IsAuthenticated());
#endif
  primary_account_info_ = signin_manager_->GetAuthenticatedAccountInfo();
  signin_manager_->AddObserver(this);
}

IdentityProxy::~IdentityProxy() {
  signin_manager_->RemoveObserver(this);
}

AccountInfo IdentityProxy::GetPrimaryAccountInfo() {
  return primary_account_info_;
}

void IdentityProxy::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void IdentityProxy::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void IdentityProxy::GoogleSigninSucceeded(const AccountInfo& account_info) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&IdentityProxy::OnPrimaryAccountLogin,
                                base::Unretained(this), account_info));
}

void IdentityProxy::GoogleSignedOut(const AccountInfo& account_info) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&IdentityProxy::OnPrimaryAccountLogout,
                                base::Unretained(this), account_info));
}

void IdentityProxy::OnPrimaryAccountLogin(const AccountInfo& account_info) {
  primary_account_info_ = account_info;
  for (auto& observer : observer_list_) {
    observer.OnPrimaryAccountLogin(account_info);
  }
}

void IdentityProxy::OnPrimaryAccountLogout(const AccountInfo& account_info) {
  DCHECK(account_info.account_id == primary_account_info_.account_id);
  DCHECK(account_info.gaia == primary_account_info_.gaia);
  DCHECK(account_info.email == primary_account_info_.email);
  primary_account_info_ = AccountInfo();
  for (auto& observer : observer_list_) {
    observer.OnPrimaryAccountLogout(account_info);
  }
}

}  // namespace identity
