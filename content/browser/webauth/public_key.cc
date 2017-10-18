// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/public_key.h"

#include "base/macros.h"

namespace content {
namespace authenticator_utils {

PublicKey::PublicKey(const std::string algorithm) : algorithm_(algorithm) {}

PublicKey::~PublicKey() {}

}  // namespace authenticator_utils
}  // namespace content
