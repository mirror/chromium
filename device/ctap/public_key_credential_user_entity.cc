// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/public_key_credential_user_entity.h"

#include <utility>

namespace device {

namespace {

// Keys for storing user entity information in CBOR map.
const char kUserIdKey[] = "id";
const char kUserNameKey[] = "name";
const char kUserDisplayNameKey[] = "displayName";
const char kUserIconUrlKey[] = "icon";

}  // namespace

PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    std::vector<uint8_t> user_id)
    : user_id_(std::move(user_id)) {}

PublicKeyCredentialUserEntity::~PublicKeyCredentialUserEntity() = default;
PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    PublicKeyCredentialUserEntity&& other) = default;
PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::operator=(
    PublicKeyCredentialUserEntity&& other) = default;
PublicKeyCredentialUserEntity::PublicKeyCredentialUserEntity(
    const PublicKeyCredentialUserEntity& other) = default;
PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::operator=(
    const PublicKeyCredentialUserEntity& other) = default;

PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::SetUserName(
    const std::string& user_name) {
  user_name_ = user_name;
  return *this;
}

PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::SetDisplayName(
    const std::string& user_display_name) {
  user_display_name_ = user_display_name;
  return *this;
}

PublicKeyCredentialUserEntity& PublicKeyCredentialUserEntity::SetIconUrl(
    const GURL& icon_url) {
  user_icon_url_ = icon_url;
  return *this;
}

cbor::CBORValue PublicKeyCredentialUserEntity::ConvertToCBOR() const {
  cbor::CBORValue::MapValue user_map;
  user_map[cbor::CBORValue(kUserIdKey)] = cbor::CBORValue(user_id_);
  if (user_name_)
    user_map[cbor::CBORValue(kUserNameKey)] = cbor::CBORValue(*user_name_);
  if (user_icon_url_)
    user_map[cbor::CBORValue(kUserIconUrlKey)] =
        cbor::CBORValue(user_icon_url_->spec());
  if (user_display_name_) {
    user_map[cbor::CBORValue(kUserDisplayNameKey)] =
        cbor::CBORValue(*user_display_name_);
  }
  return cbor::CBORValue(std::move(user_map));
}

base::Optional<PublicKeyCredentialUserEntity>
PublicKeyCredentialUserEntity::CreateFromCBORValue(
    const cbor::CBORValue& cbor) {
  if (cbor.is_map()) {
    const cbor::CBORValue::MapValue& cbor_map = cbor.GetMap();

    auto user_id = cbor_map.find(cbor::CBORValue(kUserIdKey));
    if (user_id == cbor_map.end() || !user_id->second.is_bytestring())
      return base::nullopt;
    PublicKeyCredentialUserEntity user(user_id->second.GetBytestring());

    auto user_name = cbor_map.find(cbor::CBORValue(kUserNameKey));
    if (user_name != cbor_map.end() && user_name->second.is_string()) {
      user.SetUserName(user_name->second.GetString());
    }

    auto user_display_name =
        cbor_map.find(cbor::CBORValue(kUserDisplayNameKey));
    if (user_display_name != cbor_map.end() &&
        user_display_name->second.is_string()) {
      user.SetDisplayName(user_display_name->second.GetString());
    }

    auto user_icon_url = cbor_map.find(cbor::CBORValue(kUserIconUrlKey));
    if (user_icon_url != cbor_map.end() && user_icon_url->second.is_string()) {
      user.SetIconUrl(GURL(user_icon_url->second.GetString()));
    }

    return user;
  }
  return base::nullopt;
}

}  // namespace device
