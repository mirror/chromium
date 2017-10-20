// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_COLLECTED_CLIENT_DATA_H_
#define CONTENT_BROWSER_WEBAUTH_COLLECTED_CLIENT_DATA_H_

#include <utility>
#include <vector>

#include "base/values.h"
#include "content/common/content_export.h"

namespace content {

namespace {
// JSON key values
constexpr char kChallengeKey[] = "challenge";
constexpr char kOriginKey[] = "origin";
constexpr char kHashAlg[] = "hashAlg";
constexpr char kTokenBindingKey[] = "tokenBinding";
}  // namespace

// Represents the contextual bindings of both the Relying Party and the
// client platform that is used in authenticator signatures.
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#dictdef-collectedclientdata
class CONTENT_EXPORT CollectedClientData {
 public:
  static std::unique_ptr<CollectedClientData> Create(
      const std::string relying_party_id,
      const std::vector<uint8_t> challenge);

  // Builds a JSON-serialized string per step 13 of
  // https://www.w3.org/TR/2017/WD-webauthn-20170505/#createCredential.
  std::string SerializeToJsonString();

  virtual ~CollectedClientData();

 private:
  CollectedClientData(const std::string challenge,
                      const std::string origin,
                      const std::string hash_alg,
                      const std::string token_binding_id);

  const std::string challenge_;
  const std::string origin_;
  const std::string hash_alg_;
  const std::string token_binding_id_;
  // TODO (https://crbug/757502): Add extensions support.

  DISALLOW_COPY_AND_ASSIGN(CollectedClientData);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_COLLECTED_CLIENT_DATA_H_