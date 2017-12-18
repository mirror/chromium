// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_DESCRIPTOR_H
#define DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_DESCRIPTOR_H

#include <stdint.h>
#include <string>
#include <vector>

#include "components/cbor/cbor_values.h"

namespace device {

// Data structure containing public key credential type (string) and credential
// id (byte array) as specified in the CTAP spec. Used for exclude_list for
// AuthenticatorMakeCredential command and allow_list parameter for
// AuthenticatorGetAssertion command.
class PublicKeyCredentialDescriptor {
 public:
  PublicKeyCredentialDescriptor(std::string credential_type,
                                std::vector<uint8_t> id);
  PublicKeyCredentialDescriptor(PublicKeyCredentialDescriptor&& other);
  ~PublicKeyCredentialDescriptor();
  PublicKeyCredentialDescriptor& operator=(
      PublicKeyCredentialDescriptor&& other);

  cbor::CBORValue ConvertToCBOR() const;

 private:
  std::string credential_type_;
  std::vector<uint8_t> id_;

  DISALLOW_COPY(PublicKeyCredentialDescriptor);
};

}  // namespace device

#endif  // DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_DESCRIPTOR_H
