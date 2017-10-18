// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_REGISTER_RESPONSE_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_REGISTER_RESPONSE_DATA_H_

#include "content/browser/webauth/attestation_object.h"

namespace content {

namespace authenticator_utils {

// Object containing elements from a register response to be serialized
// into PublicKeyCredentialInfo.
class RegisterResponseData {
 public:
  RegisterResponseData(const std::string& client_data_json,
                       const std::vector<uint8_t>& credential_id,
                       const std::unique_ptr<AttestationObject>& object);
  std::vector<uint8_t> GetClientDataJsonBytes();
  std::string GetId();
  std::vector<uint8_t> GetRawId();
  std::vector<uint8_t> GetCborEncodedAttestationObject();
  virtual ~RegisterResponseData();

 private:
  std::string client_data_json_;
  std::vector<uint8_t> raw_id_;
  std::unique_ptr<AttestationObject> attestation_object_;

  DISALLOW_COPY_AND_ASSIGN(RegisterResponseData);
};

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_REGISTER_RESPONSE_DATA_H_
