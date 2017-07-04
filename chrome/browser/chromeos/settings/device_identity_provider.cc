// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/settings/device_identity_provider.h"

#include "chrome/browser/chromeos/settings/device_oauth2_token_service.h"

namespace chromeos {

DeviceIdentityProvider::DeviceIdentityProvider(
    chromeos::DeviceOAuth2TokenService* token_service)
    : IdentityProvider(token_service) {}

DeviceIdentityProvider::~DeviceIdentityProvider() {}

std::string DeviceIdentityProvider::GetActiveUsername() {
  return static_cast<chromeos::DeviceOAuth2TokenService*>(GetTokenService())
      ->GetRobotAccountId();
}

std::string DeviceIdentityProvider::GetActiveAccountId() {
  return static_cast<chromeos::DeviceOAuth2TokenService*>(GetTokenService())
      ->GetRobotAccountId();
}

bool DeviceIdentityProvider::RequestLogin() {
  return false;
}

}  // namespace chromeos
