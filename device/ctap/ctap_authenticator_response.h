// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_AUTHENTICATOR_RESPONSE_H_
#define DEVICE_CTAP_CTAP_AUTHENTICATOR_RESPONSE_H_

#include "device/ctap/ctap_constants.h"

namespace device {

// Response object for AuthenticatorCancel and AuthenticatorReset commands.
class CTAPAuthenticatorResponse {
 public:
  explicit CTAPAuthenticatorResponse(
      constants::CTAPDeviceResponseCode response_code);
  CTAPAuthenticatorResponse(CTAPAuthenticatorResponse&& that);
  CTAPAuthenticatorResponse& operator=(CTAPAuthenticatorResponse&& other);
  ~CTAPAuthenticatorResponse();

  constants::CTAPDeviceResponseCode response_code() const {
    return response_code_;
  }

 private:
  constants::CTAPDeviceResponseCode response_code_;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_AUTHENTICATOR_RESPONSE_H_
