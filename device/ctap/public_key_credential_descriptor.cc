// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_descriptor.h"

namespace device {

PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    std::string credential_type,
    std::vector<uint8_t> id)
    : credential_type_(credential_type), id_(id){};
PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    PublicKeyCredentialDescriptor&& other) = default;
PublicKeyCredentialDescriptor::~PublicKeyCredentialDescriptor() = default;
PublicKeyCredentialDescriptor& PublicKeyCredentialDescriptor::operator=(
    PublicKeyCredentialDescriptor&& other) = default;

cbor::CBORValue PublicKeyCredentialDescriptor::ConvertToCBOR() const {
  cbor::CBORValue::MapValue cbor_descriptor_map;
  cbor_descriptor_map[cbor::CBORValue("id")] = cbor::CBORValue(id_);
  cbor_descriptor_map[cbor::CBORValue("type")] =
      cbor::CBORValue(credential_type_);
  return cbor::CBORValue(cbor_descriptor_map);
};

}  // namespace device
