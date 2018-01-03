// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/identity_manager.h"
#include "base/memory/ptr_util.h"
#include "base/threading/sequenced_task_runner_handle.h"

namespace identity {

IdentityManager::IdentityManager(SigninManagerBase* signin_manager,
                                 ProfileOAuth2TokenService* token_service)
    : signin_manager_(signin_manager), token_service_(token_service) {
#if defined(OS_CHROMEOS)
  // On ChromeOS, the authenticated account is set very early in startup and
  // never changed. No client should be accessing the IdentityManager before the
  // authenticated account is set.
  DCHECK(signin_manager_->IsAuthenticated());
#endif
  primary_account_info_ = signin_manager_->GetAuthenticatedAccountInfo();
  signin_manager_->AddObserver(this);
  token_service_->AddDiagnosticsObserver(this);
}

IdentityManager::~IdentityManager() {
  signin_manager_->RemoveObserver(this);
  token_service_->RemoveDiagnosticsObserver(this);
}

AccountInfo IdentityManager::GetPrimaryAccountInfo() {
  return primary_account_info_;
}

void IdentityManager::RemoveAccessTokenFromCache(
    const std::string& account_id,
    const OAuth2TokenService::ScopeSet& scopes,
    const std::string& access_token) {
  // Call PO2TS asynchronously to mimic the eventual interaction with the
  // Identity Service.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&OAuth2TokenService::InvalidateAccessToken,
                                base::Unretained(token_service_), account_id,
                                scopes, access_token));
}

std::unique_ptr<PrimaryAccountAccessTokenFetcher>
IdentityManager::CreateAccessTokenFetcherForPrimaryAccount(
    const std::string& oauth_consumer_name,
    const OAuth2TokenService::ScopeSet& scopes,
    PrimaryAccountAccessTokenFetcher::TokenCallback callback,
    PrimaryAccountAccessTokenFetcher::Mode mode) {
  return base::MakeUnique<PrimaryAccountAccessTokenFetcher>(
      oauth_consumer_name, signin_manager_, token_service_, scopes,
      std::move(callback), mode);
}

void IdentityManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void IdentityManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void IdentityManager::AddDiagnosticsObserver(DiagnosticsObserver* observer) {
  diagnostics_observer_list_.AddObserver(observer);
}

void IdentityManager::RemoveDiagnosticsObserver(DiagnosticsObserver* observer) {
  diagnostics_observer_list_.RemoveObserver(observer);
}

void IdentityManager::GoogleSigninSucceeded(const AccountInfo& account_info) {
  // Fire observer callbacks asynchronously to mimic this callback itself coming
  // in asynchronously from the Identity Service rather than synchronously from
  // SigninManager.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&IdentityManager::HandleGoogleSigninSucceeded,
                                base::Unretained(this), account_info));
}

void IdentityManager::GoogleSignedOut(const AccountInfo& account_info) {
  // Fire observer callbacks asynchronously to mimic this callback itself coming
  // in asynchronously from the Identity Service rather than synchronously from
  // SigninManager.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&IdentityManager::HandleGoogleSignedOut,
                                base::Unretained(this), account_info));
}

void IdentityManager::OnAccessTokenRequested(
    const std::string& account_id,
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes) {
  // Fire observer callbacks asynchronously to mimic this callback itself coming
  // in asynchronously from the Identity Service rather than synchronously from
  // ProfileOAuth2TokenService.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&IdentityManager::HandleOnAccessTokenRequested,
                     base::Unretained(this), account_id, consumer_id, scopes));
}

void IdentityManager::HandleGoogleSigninSucceeded(
    const AccountInfo& account_info) {
  primary_account_info_ = account_info;
  for (auto& observer : observer_list_) {
    observer.OnPrimaryAccountSet(account_info);
  }
}

void IdentityManager::HandleGoogleSignedOut(const AccountInfo& account_info) {
  DCHECK(account_info.account_id == primary_account_info_.account_id);
  DCHECK(account_info.gaia == primary_account_info_.gaia);
  DCHECK(account_info.email == primary_account_info_.email);
  primary_account_info_ = AccountInfo();
  for (auto& observer : observer_list_) {
    observer.OnPrimaryAccountCleared(account_info);
  }
}

void IdentityManager::HandleOnAccessTokenRequested(
    const std::string& account_id,
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes) {
  for (auto& observer : diagnostics_observer_list_) {
    observer.OnAccessTokenRequested(account_id, consumer_id, scopes);
  }
}

}  // namespace identity
