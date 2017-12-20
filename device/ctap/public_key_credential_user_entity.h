// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_USER_ENTITY_H
#define DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_USER_ENTITY_H

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"
#include "url/gurl.h"

namespace device {

// Data structure containing a user id, an optional user name, an optional user
// display image url, and an optional user display name as specified by the CTAP
// spec. Used as required parameter type for AuthenticatorMakeCredential
// request.
class PublicKeyCredentialUserEntity {
 public:
  PublicKeyCredentialUserEntity(std::vector<uint8_t> user_id);
  ~PublicKeyCredentialUserEntity();
  PublicKeyCredentialUserEntity(PublicKeyCredentialUserEntity&& other);
  PublicKeyCredentialUserEntity& operator=(
      PublicKeyCredentialUserEntity&& other);
  PublicKeyCredentialUserEntity Clone() const;

  void set_user_name(const std::string& user_name);
  void set_display_name(const std::string& display_name);
  void set_icon_url(const GURL& icon_url);

  const std::vector<uint8_t> get_user_id() const { return user_id_; };
  const base::Optional<std::string> get_user_name() const {
    return user_name_;
  };
  const base::Optional<std::string> get_user_display_name() const {
    return user_display_name_;
  };
  const base::Optional<GURL> get_user_icon_url() const {
    return user_icon_url_;
  };

  cbor::CBORValue ConvertToCBOR() const;
  static base::Optional<PublicKeyCredentialUserEntity> CreateFromCBORValue(
      const cbor::CBORValue& cbor);

 private:
  std::vector<uint8_t> user_id_;
  base::Optional<std::string> user_name_;
  base::Optional<std::string> user_display_name_;
  base::Optional<GURL> user_icon_url_;

  DISALLOW_COPY_AND_ASSIGN(PublicKeyCredentialUserEntity);
};

}  // namespace device

#endif  // DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_USER_ENTITY_H
