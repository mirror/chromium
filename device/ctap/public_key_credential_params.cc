// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_params.h"

namespace device {

PublicKeyCredentialParams::PublicKeyCredentialParams(
    std::list<std::map<std::string, int>> credential_params)
    : public_key_credential_param_list_(credential_params){};
PublicKeyCredentialParams::~PublicKeyCredentialParams() = default;
PublicKeyCredentialParams::PublicKeyCredentialParams(
    PublicKeyCredentialParams&& other) = default;
PublicKeyCredentialParams& PublicKeyCredentialParams::operator=(
    PublicKeyCredentialParams&& other) = default;

cbor::CBORValue PublicKeyCredentialParams::ConvertToCBOR() const {
  cbor::CBORValue::ArrayValue credential_param_array;
  for (const auto& credential_map : public_key_credential_param_list_) {
    cbor::CBORValue::MapValue cbor_credential_map;
    for (auto it = credential_map.begin(); it != credential_map.end(); ++it) {
      cbor_credential_map[cbor::CBORValue("type")] = cbor::CBORValue(it->first);
      cbor_credential_map[cbor::CBORValue("alg")] = cbor::CBORValue(it->second);
    }
    credential_param_array.emplace_back(std::move(cbor_credential_map));
  }
  return cbor::CBORValue(credential_param_array);
};

}  // namespace device
