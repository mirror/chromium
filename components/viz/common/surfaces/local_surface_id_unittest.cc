// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/local_surface_id.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(LocalSurfaceIdTest, VerifyToString) {
  base::UnguessableToken token =
      base::UnguessableToken::Deserialize(0x123, 0xABC);
  base::UnguessableToken bigToken =
      base::UnguessableToken::Deserialize(0x123456789, 0xABCABCABC);
  viz::LocalSurfaceId lsid(11, 22, token);
  viz::LocalSurfaceId bigLsid(11, 22, bigToken);

  std::string enabledExpected = ("LocalSurfaceId(11, 22, 0000012300000ABClu)");
  std::string disabledExpected = ("LocalSurfaceId(11, 22, 0123..0ABClu)");

  std::string bigEnabledExpected =
      ("LocalSurfaceId(11, 22, 123456789ABCABCABClu)");
  std::string bigDisabledExpected =
      ("LocalSurfaceId(11, 22, 123456789ABCABCABClu)");

  int tmp_log_lvl = logging::GetMinLogLevel();

  logging::SetMinLogLevel(logging::LOG_VERBOSE);
  EXPECT_EQ(enabledExpected, lsid.ToString());
  EXPECT_EQ(bigEnabledExpected, bigLsid.ToString());

  logging::SetMinLogLevel(logging::LOG_INFO);
  EXPECT_EQ(disabledExpected, lsid.ToString());
  EXPECT_EQ(bigDisabledExpected, bigLsid.ToString());

  logging::SetMinLogLevel(tmp_log_lvl);
}
