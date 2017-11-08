// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_

#include <stdint.h>
#include <vector>

#include "base/values.h"

namespace content {
namespace authenticator_utils {

// JSON key values
constexpr char kChallengeKey[] = "challenge";
constexpr char kOriginKey[] = "origin";
constexpr char kHashAlg[] = "hashAlg";
constexpr char kTokenBindingKey[] = "tokenBinding";

// const size_t kU2fResponseKeyLength = 65;

// U2FResponse byte positions
const uint32_t kU2fResponseLengthPos = 66;
const uint32_t kU2fResponseKeyHandleStartPos = 67;

constexpr char kEs256[] = "ES256";

// Helper methods for dealing with byte arrays.
void Append(std::vector<uint8_t>* target, std::vector<uint8_t> in_values);
std::vector<uint8_t> Splice(const std::vector<uint8_t>& source,
                            size_t pos,
                            size_t length);
}  // namespace authenticator_utils
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_UTILS_H_
