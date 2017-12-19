// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_descriptor.h"

namespace device {

PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    std::string credential_type,
    std::vector<uint8_t> id)
    : credential_type_(std::move(credential_type)), id_(std::move(id)){};

PublicKeyCredentialDescriptor::~PublicKeyCredentialDescriptor() = default;

PublicKeyCredentialDescriptor::PublicKeyCredentialDescriptor(
    PublicKeyCredentialDescriptor&& other) = default;

PublicKeyCredentialDescriptor& PublicKeyCredentialDescriptor::operator=(
    PublicKeyCredentialDescriptor&& other) = default;

PublicKeyCredentialDescriptor PublicKeyCredentialDescriptor::Clone() const {
  return PublicKeyCredentialDescriptor(credential_type_, id_);
}

cbor::CBORValue PublicKeyCredentialDescriptor::ConvertToCBOR() const {
  cbor::CBORValue::MapValue cbor_descriptor_map;
  cbor_descriptor_map[cbor::CBORValue("id")] = cbor::CBORValue(id_);
  cbor_descriptor_map[cbor::CBORValue("type")] =
      cbor::CBORValue(credential_type_);
  return cbor::CBORValue(std::move(cbor_descriptor_map));
};

base::Optional<PublicKeyCredentialDescriptor>
PublicKeyCredentialDescriptor::CreateFromCBORValue(
    const cbor::CBORValue& cbor) {
  const cbor::CBORValue::MapValue& map = cbor.GetMap();
  auto type = map.find(cbor::CBORValue("type"));
  if (type == map.end() || !type->second.is_string())
    return base::nullopt;

  auto id = map.find(cbor::CBORValue("id"));
  if (id == map.end() || !id->second.is_bytestring())
    return base::nullopt;

  return PublicKeyCredentialDescriptor(type->second.GetString(),
                                       id->second.GetBytestring());
};

}  // namespace device
