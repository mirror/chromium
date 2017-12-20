// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_user_entity.h"

namespace device {

PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    std::vector<uint8_t> user_id)
    : user_id_(std::move(user_id)){};

PublicKeyCredentialUserEntity::~PublicKeyCredentialUserEntity() = default;

PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    PublicKeyCredentialUserEntity&& other) = default;

PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::operator=(
    PublicKeyCredentialUserEntity&& other) = default;

PublicKeyCredentialUserEntity PublicKeyCredentialUserEntity::Clone() const {
  auto user = PublicKeyCredentialUserEntity(user_id_);
  if (user_name_)
    user.set_user_name(user_name_.value());
  if (user_display_name_)
    user.set_display_name(user_display_name_.value());
  if (user_icon_url_)
    user.set_icon_url(user_icon_url_.value());
  return user;
}

void PublicKeyCredentialUserEntity::set_user_name(
    const std::string& user_name) {
  user_name_ = user_name;
};

void PublicKeyCredentialUserEntity::set_display_name(
    const std::string& user_display_name) {
  user_display_name_ = user_display_name;
};

void PublicKeyCredentialUserEntity::set_icon_url(const GURL& icon_url) {
  user_icon_url_ = icon_url;
};

cbor::CBORValue PublicKeyCredentialUserEntity::ConvertToCBOR() const {
  cbor::CBORValue::MapValue user_map;
  user_map[cbor::CBORValue("id")] = cbor::CBORValue(user_id_);
  if (user_name_)
    user_map[cbor::CBORValue("name")] = cbor::CBORValue(*user_name_);
  if (user_icon_url_)
    user_map[cbor::CBORValue("icon")] = cbor::CBORValue(user_icon_url_->spec());
  if (user_display_name_) {
    user_map[cbor::CBORValue("displayName")] =
        cbor::CBORValue(*user_display_name_);
  }
  return cbor::CBORValue(user_map);
};

base::Optional<PublicKeyCredentialUserEntity>
PublicKeyCredentialUserEntity::CreateFromCBORValue(
    const cbor::CBORValue& cbor) {
  if (cbor.is_map() && cbor.GetMap().count(cbor::CBORValue("id"))) {
    const cbor::CBORValue::MapValue& cbor_map = cbor.GetMap();

    PublicKeyCredentialUserEntity user(
        cbor_map.find(cbor::CBORValue("id"))->second.GetBytestring());
    if (cbor_map.count(cbor::CBORValue("name"))) {
      user.set_user_name(
          cbor_map.find(cbor::CBORValue("name"))->second.GetString());
    }
    if (cbor_map.count(cbor::CBORValue("displayName"))) {
      user.set_display_name(
          cbor_map.find(cbor::CBORValue("displayName"))->second.GetString());
    }
    if (cbor_map.count(cbor::CBORValue("icon"))) {
      user.set_icon_url(
          GURL(cbor_map.find(cbor::CBORValue("icon"))->second.GetString()));
    }
    return user;
  }
  return base::nullopt;
};

}  // namespace device
