// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_constants.h"

namespace device {

const char kResidentKeyMapKey[] = "rk";
const char kUserVerificationMapKey[] = "uv";
const char kUserPresenceMapKey[] = "up";

const std::vector<uint8_t> kU2FBogusRegisterApplicationParam =
    std::vector<uint8_t>(32, 0x41);
const std::vector<uint8_t> kU2FBogusRegisterChallengeParam =
    std::vector<uint8_t>(32, 0x42);

const base::TimeDelta kDelayKeepAlive = base::TimeDelta::FromMilliseconds(100);

const size_t kPacketSize = 64;
const uint32_t kHIDBroadcastChannel = 0xffffffff;
const size_t kInitPacketHeader = 7;
const size_t kContinuationPacketHeader = 5;
const size_t kMaxHidPacketSize = 64;
const size_t kInitPacketDataSize = kMaxHidPacketSize - kInitPacketHeader;
const size_t kContinuationPacketDataSize =
    kMaxHidPacketSize - kContinuationPacketHeader;
const size_t kMaxMessageSize = 7609;
const uint8_t kReportId = 0x00;
const uint8_t kWinkCapability = 0x01;
const uint8_t kLockCapability = 0x02;

}  // namespace device
