// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/mac/seatbelt_extension_token.h"

#include "base/memory/ptr_util.h"

namespace sandbox {

SeatbeltExtensionToken::SeatbeltExtensionToken() {}

SeatbeltExtensionToken::~SeatbeltExtensionToken() {}

// static
std::unique_ptr<SeatbeltExtensionToken> SeatbeltExtensionToken::NewForTesting(
    const std::string& fake_token) {
  return base::WrapUnique(new SeatbeltExtensionToken(fake_token));
}

SeatbeltExtensionToken::SeatbeltExtensionToken(const std::string& token)
    : token_(token) {}

}  // namespace sandbox
