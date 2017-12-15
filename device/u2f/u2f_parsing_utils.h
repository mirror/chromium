// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_AUTHENTICATOR_UTILS_H_
#define DEVICE_U2F_AUTHENTICATOR_UTILS_H_

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace device {
namespace u2f_parsing_utils {

// U2FResponse byte positions
constexpr uint32_t kU2fResponseLengthPos = 66;
constexpr uint32_t kU2fResponseKeyHandleStartPos = 67;

constexpr char kEs256[] = "ES256";

void Append(std::vector<uint8_t>* target,
            const std::vector<uint8_t>& in_values);

// Parses out a sub-vector after verifying no out-of-bound reads.
std::vector<uint8_t> Extract(const std::vector<uint8_t>& source,
                             size_t pos,
                             size_t length);

}  // namespace u2f_parsing_utils
}  // namespace device

#endif  // DEVICE_U2F_AUTHENTICATOR_UTILS_H_
