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

namespace device {

class PublicKeyCredentialUserEntity {
 public:
  PublicKeyCredentialUserEntity(std::vector<uint8_t> user_id);
  ~PublicKeyCredentialUserEntity();
  PublicKeyCredentialUserEntity(PublicKeyCredentialUserEntity&& other);
  PublicKeyCredentialUserEntity& operator=(
      PublicKeyCredentialUserEntity&& other);

  void SetUserName(std::string user_name);
  void SetUserDisplayName(std::string display_name);
  void SetUserIconUrl(std::string icon_url);
  cbor::CBORValue ConvertToCBOR() const;

 private:
  std::vector<uint8_t> user_id_;
  base::Optional<std::string> user_name_;
  base::Optional<std::string> user_display_name_;
  base::Optional<std::string> user_icon_url_;

  DISALLOW_COPY(PublicKeyCredentialUserEntity);
};

}  // namespace device

#endif  // DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_USER_ENTITY_H
