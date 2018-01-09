// Copyright 2017 The Chromium Authors. All rights reserved.
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

AuthenticatorOptions& AuthenticatorOptions::SetPlatformDevice(
    bool platform_device) {
  platform_device_ = platform_device;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetResidentKey(bool resident_key) {
  resident_key_ = resident_key;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetUserVerification(
    bool user_verification) {
  user_verification_ = user_verification;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetUserPresence(
    bool user_presence) {
  user_presence_ = user_presence;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetClientPin(bool client_pin) {
  client_pin_ = client_pin;
  return *this;
}

}  // namespace device
