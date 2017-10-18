// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_

#include <vector>

#include "base/values.h"
#include "content/browser/webauth/register_response_data.h"
#include "third_party/WebKit/public/platform/modules/webauth/authenticator.mojom.h"

namespace content {

namespace authenticator_utils {

// JSON key values
constexpr char kChallengeKey[] = "challenge";
constexpr char kOriginKey[] = "origin";
constexpr char kHashAlg[] = "hashAlg";
constexpr char kTokenBindingKey[] = "tokenBinding";

// Build client_data per step 13 of
// https://www.w3.org/TR/2017/WD-webauthn-20170505/#createCredential).
std::string CONTENT_EXPORT
BuildClientData(const std::string& relying_party_id,
                const std::vector<uint8_t>& challenge);

// Extracts and repackages the data from the response returned from a U2F
// authenticator, along with |clientDataJson|, that are needed to produce
// a response to a navigator.credentials.create request.
std::unique_ptr<RegisterResponseData> ParseU2fRegisterResponse(
    const std::string& client_data_json,
    const std::vector<uint8_t>& data);

}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_