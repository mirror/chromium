// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_options.h"

#include <utility>

namespace device {

AuthenticatorOptions::AuthenticatorOptions()
    : platform_device_(false), rk_(false), uv_(false), up_(true) {}

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

AuthenticatorOptions& AuthenticatorOptions::SetRK(bool rk) {
  rk_ = rk;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetUV(bool uv) {
  uv_ = uv;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetUP(bool up) {
  up_ = up;
  return *this;
}

AuthenticatorOptions& AuthenticatorOptions::SetClientPin(bool client_pin) {
  client_pin_ = client_pin;
  return *this;
}

}  // namespace device
