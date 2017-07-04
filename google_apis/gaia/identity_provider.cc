// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/profiler/scoped_tracker.h"
#include "google_apis/gaia/identity_provider.h"

IdentityProvider::Observer::~Observer() {}

IdentityProvider::IdentityProvider(OAuth2TokenService* token_service)
    : token_service_(token_service) {
  DCHECK(token_service_);
  token_service_->AddObserver(this);
}

IdentityProvider::~IdentityProvider() {
  token_service_->RemoveObserver(this);
}

void IdentityProvider::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void IdentityProvider::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void IdentityProvider::OnRefreshTokenAvailable(const std::string& account_id) {
  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 IdentityProvider::OnRefreshTokenAvailable"));

  if (account_id != GetActiveAccountId())
    return;
  for (auto& observer : observers_)
    observer.OnRefreshTokenAvailableForActiveAccount();
}

void IdentityProvider::OnRefreshTokenRevoked(const std::string& account_id) {
  if (account_id != GetActiveAccountId())
    return;
  for (auto& observer : observers_)
    observer.OnRefreshTokenRevokedForActiveAccount();
}

void IdentityProvider::FireOnActiveAccountLogin() {
  for (auto& observer : observers_)
    observer.OnActiveAccountLogin();
}

void IdentityProvider::FireOnActiveAccountLogout() {
  for (auto& observer : observers_)
    observer.OnActiveAccountLogout();
}
