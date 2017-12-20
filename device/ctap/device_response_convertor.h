// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_DEVICE_RESPONSE_CONVERTOR_H_
#define DEVICE_CTAP_DEVICE_RESPONSE_CONVERTOR_H_

#include <stdint.h>
#include <vector>

#include "base/optional.h"
#include "device/ctap/authenticator_get_assertion_response.h"
#include "device/ctap/authenticator_get_info_response.h"
#include "device/ctap/authenticator_make_credential_response.h"

namespace device {

namespace response_convertor {

// De-serializes CBOR encoded response to AuthenticatorMakeCredential request
// as specified in https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-
// client-to-authenticator-protocol-v2.0-rd-20170927.html
//
// Returns null optional if any of the encoding formats are incorrect.
base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(const std::vector<uint8_t>& buffer);

// De-serializes CBOR encoded response to AuthenticatorGetAssertion request as
// defined by the CTAP protocol.
base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    const std::vector<uint8_t>& buffer);

// De-serializes CBOR encoded response to AuthenticatorGetInfo request as
// defined by the CTAP protocol.
base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponse(
    const std::vector<uint8_t>& buffer);

}  // namespace response_convertor

}  // namespace device

#endif  // DEVICE_CTAP_DEVICE_RESPONSE_CONVERTOR_H_
