// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ownership/owner_key_util.h"

#include <utility>

namespace ownership {

///////////////////////////////////////////////////////////////////////////
// PublicKey

PublicKey::PublicKey() = default;

PublicKey::~PublicKey() = default;

///////////////////////////////////////////////////////////////////////////
// PrivateKey

PrivateKey::PrivateKey(crypto::ScopedSECKEYPrivateKey key)
    : key_(std::move(key)) {}

PrivateKey::~PrivateKey() = default;

}  // namespace ownership
