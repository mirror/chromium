// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_options.h"

#include <utility>

namespace device {

AuthenticatorOptions::AuthenticatorOptions() = default;

AuthenticatorOptions::AuthenticatorOptions(AuthenticatorOptions&& other) =
    default;

AuthenticatorOptions& AuthenticatorOptions::operator=(
    AuthenticatorOptions&& other) = default;

AuthenticatorOptions::~AuthenticatorOptions() = default;

AuthenticatorOptions& AuthenticatorOptions::SetIsPlatformDevice(
    bool is_platform_device) {
  is_platform_device_ = is_platform_device;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetSupportsResidentKey(
    bool supports_resident_key) {
  supports_resident_key_ = supports_resident_key;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetUserVerificationRequired(
    bool user_verification_required) {
  user_verification_required_ = user_verification_required;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetUserPresenceRequired(
    bool user_presence_required) {
  user_presence_required_ = user_presence_required;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetClientPinStored(
    bool client_pin_stored) {
  client_pin_stored_ = client_pin_stored;
  return *this;
}

}  // namespace device
