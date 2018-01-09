// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_
#define DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_

#include <string>

#include "base/macros.h"
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
  AuthenticatorOptions& SetUserVerificationRequired(
      bool user_verification_required);
  AuthenticatorOptions& SetUserPresenceRequired(bool user_presence_required);
  AuthenticatorOptions& SetClientPinStored(bool client_pin_stored);

  bool platform_device() const { return platform_device_; }
  bool resident_key() const { return resident_key_; }
  bool user_verification_required() const {
    return user_verification_required_;
  }
  bool user_presence_required() const { return user_presence_required_; }
  const base::Optional<bool>& client_pin_stored() const {
    return client_pin_stored_;
  }

 private:
  bool platform_device_ = false;
  bool resident_key_ = false;
  bool user_verification_required_ = false;
  bool user_presence_required_ = true;
  base::Optional<bool> client_pin_stored_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorOptions);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_OPTIONS_H_
