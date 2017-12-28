// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_
#define DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_

#include <string>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"

namespace device {

// Data structure containing authenticator options to be used as request
// request parameters for AuthenticatorMakeCredential command and
// AuthenticatorGetAssertion command and response parameter for
// AuthenticatorGetInfo command.
class AuthenticatorOptions {
 public:
  AuthenticatorOptions();
  AuthenticatorOptions(AuthenticatorOptions&& other);
  AuthenticatorOptions& operator=(AuthenticatorOptions&& other);
  ~AuthenticatorOptions();

  cbor::CBORValue ConvertToCBOR() const;
  AuthenticatorOptions& SetPlatformDevice(const bool platform_device);
  AuthenticatorOptions& SetRK(const bool rk);
  AuthenticatorOptions& SetUV(const bool uv);
  AuthenticatorOptions& SetUP(const bool is_platform_device);
  AuthenticatorOptions& SetClientPin(const bool client_pin);

  bool platform_device() const { return platform_device_; }
  bool rk() const { return rk_; }
  bool uv() const { return uv_; }
  bool up() const { return up_; }
  const base::Optional<bool>& client_pin() const { return client_pin_; }

 private:
  bool platform_device_;
  bool rk_;
  bool uv_;
  bool up_;
  base::Optional<bool> client_pin_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorOptions);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_
