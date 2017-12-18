// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_RP_ENTITY_H
#define DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_RP_ENTITY_H

#include <string>
#include <vector>

#include "base/optional.h"
#include "components/cbor/cbor_values.h"

namespace device {

// Data structure containing information about relying party that invoked
// WebAuth API. Includes a relying party id, an optional relying party name,,
// and optional relying party display image url.
class PublicKeyCredentialRPEntity {
 public:
  PublicKeyCredentialRPEntity(std::string rp_id);
  ~PublicKeyCredentialRPEntity();
  PublicKeyCredentialRPEntity(PublicKeyCredentialRPEntity&& other);
  PublicKeyCredentialRPEntity& operator=(PublicKeyCredentialRPEntity&& other);

  void SetRPName(std::string rp_name);
  void SetRPIconUrl(std::string icon_url);
  cbor::CBORValue ConvertToCBOR() const;

 private:
  std::string rp_id_;
  base::Optional<std::string> rp_name_;
  base::Optional<std::string> rp_icon_url_;

  DISALLOW_COPY(PublicKeyCredentialRPEntity);
};

}  // namespace device

#endif  // DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_RP_ENTITY_H
