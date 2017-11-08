// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_utils.h"

namespace content {
namespace authenticator_utils {

void Append(std::vector<uint8_t>* target, std::vector<uint8_t> in_values) {
  target->insert(target->end(), in_values.begin(), in_values.end());
}

std::vector<uint8_t> Splice(const std::vector<uint8_t>& source,
                            size_t pos,
                            size_t length) {
  CHECK_GE(source.size(), pos + length);
  return std::vector<uint8_t>(&source[pos], &source[pos + length]);
}

}  // namespace authenticator_utils
}  // namespace content