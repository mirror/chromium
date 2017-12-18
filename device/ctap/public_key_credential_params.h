// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_PARAMS_H
#define DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_PARAMS_H

#include <list>
#include <map>
#include <string>

#include "components/cbor/cbor_values.h"

namespace device {

class PublicKeyCredentialParams {
 public:
  PublicKeyCredentialParams(
      std::list<std::map<std::string, int>> credential_params);
  ~PublicKeyCredentialParams();
  PublicKeyCredentialParams(PublicKeyCredentialParams&& other);
  PublicKeyCredentialParams& operator=(PublicKeyCredentialParams&& other);

  cbor::CBORValue ConvertToCBOR() const;

 private:
  std::list<std::map<std::string, int>> public_key_credential_param_list_;

  DISALLOW_COPY(PublicKeyCredentialParams);
};

}  // namespace device

#endif  // DEVICE_CTAP_PUBLIC_KEY_CREDENTIAL_PARAMS_H
