// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_
#define DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_

#include <string>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"

namespace device {

class AuthenticatorOptions {
 public:
  AuthenticatorOptions();
  AuthenticatorOptions(AuthenticatorOptions&& other);
  AuthenticatorOptions& operator=(AuthenticatorOptions&& other);
  ~AuthenticatorOptions();

  cbor::CBORValue ConvertToCBOR() const;
  AuthenticatorOptions& SetPlatformDevice(bool platform_device);
  AuthenticatorOptions& SetResidentKey(bool resident_key);
  AuthenticatorOptions& SetUserVerification(bool user_verification);
  AuthenticatorOptions& SetUserPresence(bool is_platform_device);
  AuthenticatorOptions& SetClientPin(bool client_pin);

  bool platform_device() const { return platform_device_; }
  bool resident_key() const { return resident_key_; }
  bool user_verification() const { return user_verification_; }
  bool user_presence() const { return user_presence_; }
  const base::Optional<bool>& client_pin() const { return client_pin_; }

 private:
  bool platform_device_ = false;
  bool resident_key_ = false;
  bool user_verification_ = false;
  bool user_presence_ = true;
  base::Optional<bool> client_pin_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorOptions);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_
