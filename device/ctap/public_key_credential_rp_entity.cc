// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_rp_entity.h"

namespace device {

PublicKeyCredentialRPEntity::PublicKeyCredentialRPEntity(std::string rp_id)
    : rp_id_(std::move(rp_id)){};

PublicKeyCredentialRPEntity::~PublicKeyCredentialRPEntity() = default;

PublicKeyCredentialRPEntity::PublicKeyCredentialRPEntity(
    PublicKeyCredentialRPEntity&& other) = default;

PublicKeyCredentialRPEntity& PublicKeyCredentialRPEntity::operator=(
    PublicKeyCredentialRPEntity&& other) = default;

void PublicKeyCredentialRPEntity::set_rp_name(const std::string& rp_name) {
  rp_name_ = rp_name;
};
void PublicKeyCredentialRPEntity::set_rp_icon_url(const GURL& icon_url) {
  rp_icon_url_ = icon_url;
};
cbor::CBORValue PublicKeyCredentialRPEntity::ConvertToCBOR() const {
  cbor::CBORValue::MapValue rp_map;
  rp_map[cbor::CBORValue("id")] = cbor::CBORValue(rp_id_);
  if (rp_name_.has_value())
    rp_map[cbor::CBORValue("name")] = cbor::CBORValue(rp_name_.value());
  if (rp_icon_url_.has_value())
    rp_map[cbor::CBORValue("icon")] =
        cbor::CBORValue(rp_icon_url_.value().spec());
  return cbor::CBORValue(rp_map);
};

}  // namespace device
