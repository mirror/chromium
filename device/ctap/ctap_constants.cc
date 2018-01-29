// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_constants.h"

namespace device {

const char kResidentKeyMapKey[] = "rk";
const char kUserVerificationMapKey[] = "uv";
const char kUserPresenceMapKey[] = "up";

const size_t kPacketSize = 64;
const uint32_t kHIDBroadcastChannel = 0xffffffff;
const size_t kInitPacketHeader = 7;
const size_t kContinuationPacketHeader = 5;
const size_t kMaxHidPacketSize = 64;
const size_t kInitPacketDataSize = kMaxHidPacketSize - kInitPacketHeader;
const size_t kContinuationPacketDataSize =
    kMaxHidPacketSize - kContinuationPacketHeader;
const uint8_t kMaxHIDLockSeconds = 10;
const size_t kMaxMessageSize = 7609;

}  // namespace device
