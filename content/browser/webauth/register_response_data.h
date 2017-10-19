// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_REGISTER_RESPONSE_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_REGISTER_RESPONSE_DATA_H_

#include "content/browser/webauth/attestation_object.h"
#include "content/browser/webauth/collected_client_data.h"

namespace content {

// Unpacks a U2F register response and extracts the elements to be serialized
// into PublicKeyCredentialInfo.
// TODO: Link to the U2F-to-CTAP diagrams in the FIDO 2.0 spec once published.
class RegisterResponseData {
 public:
  static std::unique_ptr<RegisterResponseData> Create(
      std::unique_ptr<CollectedClientData> client_data,
      const std::vector<uint8_t> u2f_data);

  std::vector<uint8_t> GetClientDataJsonBytes();
  std::string GetId();
  std::vector<uint8_t> GetRawId();
  std::vector<uint8_t> GetCborEncodedAttestationObject();
  virtual ~RegisterResponseData();

 private:
  RegisterResponseData(std::unique_ptr<CollectedClientData> client_data,
                       const std::vector<uint8_t> credential_id,
                       std::unique_ptr<AttestationObject> object);

  const std::unique_ptr<CollectedClientData> client_data_;
  const std::vector<uint8_t> raw_id_;
  const std::unique_ptr<AttestationObject> attestation_object_;

  DISALLOW_COPY_AND_ASSIGN(RegisterResponseData);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_REGISTER_RESPONSE_DATA_H_
