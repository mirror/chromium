// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/local_surface_id.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(LocalSurfaceIdTest, VerifyToString) {
  base::UnguessableToken token = base::UnguessableToken::Deserialize(0x123, 0xABC);
  base::UnguessableToken bigToken = base::UnguessableToken::Deserialize(0x123456789, 0xABCABCABC);
  viz::LocalSurfaceId lsid(11, 22, token);
  viz::LocalSurfaceId bigLsid(11, 22, bigToken);

  std::string enabledExpected = ("LocalSurfaceId(11, 22, 0000012300000ABC)");
  std::string disabledExpected = ("LocalSurfaceId(11, 22, 0123..0ABC)");

  std::string bigEnabledExpected = ("LocalSurfaceId(11, 22, 123456789ABCABCABC)");
  std::string bigDisabledExpected = ("LocalSurfaceId(11, 22, 123456789ABCABCABC)");

  Vlog_Info* tmp_log_info = g_vlog_info;
  int tmp_log_lvl = g_min_log_level;

  g_vlog_info = nullptr;

  g_min_log_level = -1;
  EXPECT_EQ(enabledExpected, lsid.ToString());
  EXPECT_EQ(bigEnabledExpected, bigLsid.ToString());

  g_min_log_level = 1;
  EXPECT_EQ(disabledExpected, lsid.ToString());
  EXPECT_EQ(bigDisabledExpected, bigLsid.ToString());

  g_vlog_info = std::move(tmp_log_info);
  g_min_log_level = tmp_log_lvl;
}
