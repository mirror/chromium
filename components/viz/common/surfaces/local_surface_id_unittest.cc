// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/surfaces/local_surface_id.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

// Verifying that Local_Surface_Id::ToString() prints its
// UnguessableToken as ABCD.. if logging is not verbose and prints
// full token otherwise
TEST(LocalSurfaceIdTest, VerifyToString) {
  base::UnguessableToken token =
      base::UnguessableToken::Deserialize(0x111111, 0);
  base::UnguessableToken bigToken =
      base::UnguessableToken::Deserialize(0x123456789, 0xABCABCABC);
  base::UnguessableToken smallToken =
      base::UnguessableToken::Deserialize(0, 0x1);
  viz::LocalSurfaceId lsid(11, 22, token);
  viz::LocalSurfaceId bigLsid(11, 22, bigToken);
  viz::LocalSurfaceId smallLsid(11, 22, smallToken);

  std::string verboseExpected = ("LocalSurfaceId(11, 22, 0011111100000000)");
  std::string shyExpected = ("LocalSurfaceId(11, 22, 0011..)");

  std::string bigVerboseExpected =
      ("LocalSurfaceId(11, 22, 123456789ABCABCABC)");
  std::string bigShyExpected = ("LocalSurfaceId(11, 22, 1234..)");

  std::string smallVerboseExpected =
      ("LocalSurfaceId(11, 22, 0000000000000001)");
  std::string smallShyExpected = ("LocalSurfaceId(11, 22, 0000..)");

  int tmp_log_lvl = logging::GetMinLogLevel();

  // When |g_min_log_level| is set to LOG_VERBOSE we expect
  // verbose versions of local_surface_id::ToString()
  logging::SetMinLogLevel(logging::LOG_VERBOSE);
  EXPECT_EQ(verboseExpected, lsid.ToString());
  EXPECT_EQ(bigVerboseExpected, bigLsid.ToString());
  EXPECT_EQ(smallVerboseExpected, smallLsid.ToString());

  // When |g_min_log_level| is set to LOG_INFO we expect
  // less verbose versions of local_surface_id::ToString()
  logging::SetMinLogLevel(logging::LOG_INFO);
  EXPECT_EQ(shyExpected, lsid.ToString());
  EXPECT_EQ(bigShyExpected, bigLsid.ToString());
  EXPECT_EQ(smallShyExpected, smallLsid.ToString());

  logging::SetMinLogLevel(tmp_log_lvl);
}
