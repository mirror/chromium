// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_DESCRIPTOR_H_
#define DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_DESCRIPTOR_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
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
  ~PublicKeyCredentialDescriptor();
  PublicKeyCredentialDescriptor(PublicKeyCredentialDescriptor&& other);
  PublicKeyCredentialDescriptor& operator=(
      PublicKeyCredentialDescriptor&& other);
  PublicKeyCredentialDescriptor(const PublicKeyCredentialDescriptor& other);
  PublicKeyCredentialDescriptor& operator=(
      const PublicKeyCredentialDescriptor& other);

  const std::string& credential_type() const { return credential_type_; }
  const std::vector<uint8_t>& credential_id() const { return id_; }

  cbor::CBORValue ConvertToCBOR() const;
  static base::Optional<PublicKeyCredentialDescriptor> CreateFromCBORValue(
      const cbor::CBORValue& cbor);

 private:
  std::string credential_type_;
  std::vector<uint8_t> id_;
};

}  // namespace device

#endif  // DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_DESCRIPTOR_H_
