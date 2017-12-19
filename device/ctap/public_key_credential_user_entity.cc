// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_user_entity.h"

namespace device {

PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    std::vector<uint8_t> user_id)
    : user_id_(user_id){};

PublicKeyCredentialUserEntity::~PublicKeyCredentialUserEntity() = default;

PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    PublicKeyCredentialUserEntity&& other) = default;

PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::operator=(
    PublicKeyCredentialUserEntity&& other) = default;

PublicKeyCredentialUserEntity PublicKeyCredentialUserEntity::Clone() const {
  auto user = PublicKeyCredentialUserEntity(user_id_);
  if (user_name_)
    user.SetUserName(user_name_.value());
  if (user_display_name_)
    user.SetUserDisplayName(user_display_name_.value());
  if (user_icon_url_)
    user.SetUserIconUrl(user_icon_url_.value());
  return user;
}

void PublicKeyCredentialUserEntity::SetUserName(std::string user_name) {
  user_name_ = user_name;
};

void PublicKeyCredentialUserEntity::SetUserDisplayName(
    std::string user_display_name) {
  user_display_name_ = user_display_name;
};

void PublicKeyCredentialUserEntity::SetUserIconUrl(std::string icon_url) {
  user_icon_url_ = icon_url;
};

cbor::CBORValue PublicKeyCredentialUserEntity::ConvertToCBOR() const {
  cbor::CBORValue::MapValue user_map;
  user_map[cbor::CBORValue("id")] = cbor::CBORValue(user_id_);
  if (user_name_.has_value())
    user_map[cbor::CBORValue("name")] = cbor::CBORValue(user_name_.value());
  if (user_icon_url_.has_value())
    user_map[cbor::CBORValue("icon")] = cbor::CBORValue(user_icon_url_.value());
  if (user_display_name_.has_value()) {
    user_map[cbor::CBORValue("displayName")] =
        cbor::CBORValue(user_display_name_.value());
  }
  return cbor::CBORValue(user_map);
};

base::Optional<PublicKeyCredentialUserEntity>
PublicKeyCredentialUserEntity::CreateFromCBORValue(
    const cbor::CBORValue& cbor) {
  if (cbor.is_map() && cbor.GetMap().count(cbor::CBORValue("id"))) {
    PublicKeyCredentialUserEntity user(
        cbor.GetMap().find(cbor::CBORValue("id"))->second.GetBytestring());
    if (cbor.GetMap().count(cbor::CBORValue("name"))) {
      user.SetUserName(
          cbor.GetMap().find(cbor::CBORValue("name"))->second.GetString());
    }
    if (cbor.GetMap().count(cbor::CBORValue("displayName"))) {
      user.SetUserName(cbor.GetMap()
                           .find(cbor::CBORValue("displayName"))
                           ->second.GetString());
    }
    if (cbor.GetMap().count(cbor::CBORValue("icon"))) {
      user.SetUserName(
          cbor.GetMap().find(cbor::CBORValue("icon"))->second.GetString());
    }
    return user;
  }
  return base::nullopt;
};

}  // namespace device
