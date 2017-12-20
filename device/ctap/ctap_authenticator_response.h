// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_AUTHENTICATOR_RESPONSE_H_
#define DEVICE_CTAP_CTAP_AUTHENTICATOR_RESPONSE_H_

#include "device/ctap/ctap_response.h"
#include "device/ctap/ctap_response_code.h"

namespace device {

// Response object for AuthenticatorCancel and AuthenticatorReset command
class CTAPAuthenticatorResponse : public CTAPResponse {
 public:
  CTAPAuthenticatorResponse(CTAPResponseCode response_code);
  ~CTAPAuthenticatorResponse() override;
  CTAPAuthenticatorResponse(CTAPAuthenticatorResponse&& that);

 private:
  DISALLOW_COPY_AND_ASSIGN(CTAPAuthenticatorResponse);
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_AUTHENTICATOR_RESPONSE_H_
