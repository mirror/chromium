// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_version_param.h"

#include <utility>

#include "components/apdu/apdu_command.h"
#include "device/fido/ctap_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

TEST(U2fVersionParamTest, TestEncodeVersionRequest) {
  U2fVersionParam version_parameter;
  auto encoded_version = version_parameter.Encode();
  const uint8_t kEncodedU2fVersionRequest[] = {0x00, kInsU2fVersion, 0x00, 0x00,
                                               0x00, 0x00,           0x00};

  ASSERT_TRUE(encoded_version);
  EXPECT_THAT(encoded_version, testing::ContainerEq(kEncodedU2fVersionRequest));
  const auto version_cmd =
      apdu::ApduCommand::CreateFromMessageForTesting(*encoded_version);
  ASSERT_TRUE(version_cmd);
  EXPECT_THAT(version_cmd->GetEncodedCommand(),
              testing::ContainerEq(kEncodedU2fVersionRequest));
}

TEST(U2fVersionParamTest, TestEncodeLegacyVersionRequest) {
  U2fVersionParam legacy_version_parameter;
  legacy_version_parameter.SetIsLegacyVersion(true);
  auto encoded_legacy_version = legacy_version_parameter.Encode();
  // Legacy version command contains 2 extra null bytes compared to ISO 7816-4
  // format.
  const uint8_t kEncodedU2fVersionRequest[] = {
      0x00, kInsU2fVersion, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  ASSERT_TRUE(encoded_legacy_version);
  EXPECT_THAT(encoded_legacy_version,
              testing::ContainerEq(kEncodedU2fVersionRequest));
  const auto legacy_version_cmd =
      apdu::ApduCommand::CreateFromMessageForTesting(*encoded_legacy_version);
  ASSERT_TRUE(legacy_version_cmd);
  EXPECT_THAT(legacy_version_cmd->GetEncodedCommand(),
              testing::ContainerEq(kEncodedU2fVersionRequest));
}

}  // namespace device
