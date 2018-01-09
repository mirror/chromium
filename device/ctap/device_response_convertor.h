// Copyright 2018 The Chromium Authors. All rights reserved.
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
#include "device/ctap/ctap_constants.h"

namespace device {

// Converts response from authenticators to CTAPResponse objects. If the
// response of the authenticator does not conform to format specified by the
// CTAP protocol, null optional is returned.
namespace response_convertor {

// Parses response code from  response received from the authenticator. If
// unknown response code value is received, then CTAP2_ERR_OTHER is returned.
constants::CTAPDeviceResponseCode GetResponseCode(
    const std::vector<uint8_t>& buffer);

// De-serializes CBOR encoded response to AuthenticatorMakeCredential request
// to AuthenticatorMakeCredentialResponse object.
base::Optional<AuthenticatorMakeCredentialResponse>
ReadCTAPMakeCredentialResponse(constants::CTAPDeviceResponseCode response_code,
                               const std::vector<uint8_t>& buffer);

// De-serializes CBOR encoded response to AuthenticatorGetAssertion /
// AuthenticatorGetNextAssertion request to AuthenticatorGetAssertionResponse
// object.
base::Optional<AuthenticatorGetAssertionResponse> ReadCTAPGetAssertionResponse(
    constants::CTAPDeviceResponseCode response_code,
    const std::vector<uint8_t>& buffer);

// De-serializes CBOR encoded response to AuthenticatorGetInfo request to
// AuthenticatorGetInfoResponse object.
base::Optional<AuthenticatorGetInfoResponse> ReadCTAPGetInfoResponse(
    constants::CTAPDeviceResponseCode response_code,
    const std::vector<uint8_t>& buffer);

}  // namespace response_convertor

}  // namespace device

#endif  // DEVICE_CTAP_DEVICE_RESPONSE_CONVERTOR_H_
