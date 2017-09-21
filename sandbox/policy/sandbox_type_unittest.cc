// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/policy/sandbox_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {

TEST(SandboxTypeTest, IsUnsandboxed) {
  // TODO(tsepez): network service should be sandboxed.
  EXPECT_TRUE(IsUnsandboxedSandboxType(SANDBOX_TYPE_NO_SANDBOX));
  EXPECT_TRUE(IsUnsandboxedSandboxType(SANDBOX_TYPE_NETWORK));

  EXPECT_FALSE(IsUnsandboxedSandboxType(SANDBOX_TYPE_RENDERER));
  EXPECT_FALSE(IsUnsandboxedSandboxType(SANDBOX_TYPE_UTILITY));
  EXPECT_FALSE(IsUnsandboxedSandboxType(SANDBOX_TYPE_GPU));
  EXPECT_FALSE(IsUnsandboxedSandboxType(SANDBOX_TYPE_PPAPI));
  EXPECT_FALSE(IsUnsandboxedSandboxType(SANDBOX_TYPE_WIDEVINE));
}

TEST(SandboxTypeTest, ToString) {
  // Only the subset of policies that apply (theoretically) to services
  // can be converted to strings, with the default being "utility".
  EXPECT_EQ("none", SandboxTypeToString(SANDBOX_TYPE_NO_SANDBOX));
  EXPECT_EQ("network", SandboxTypeToString(SANDBOX_TYPE_NETWORK));
  EXPECT_EQ("ppapi", SandboxTypeToString(SANDBOX_TYPE_PPAPI));
  EXPECT_EQ("widevine", SandboxTypeToString(SANDBOX_TYPE_WIDEVINE));
  EXPECT_EQ("utility", SandboxTypeToString(SANDBOX_TYPE_UTILITY));
}

TEST(SandboxTypeTest, FromString) {
  // Only the subset of policies that apply (theoretically) to services
  // can be extracted from strings, with the default being utility.
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromString("none"));
  EXPECT_EQ(SANDBOX_TYPE_NETWORK, SandboxTypeFromString("network"));
  EXPECT_EQ(SANDBOX_TYPE_PPAPI, SandboxTypeFromString("ppapi"));
  EXPECT_EQ(SANDBOX_TYPE_WIDEVINE, SandboxTypeFromString("widevine"));
  EXPECT_EQ(SANDBOX_TYPE_UTILITY, SandboxTypeFromString("utility"));
  EXPECT_EQ(SANDBOX_TYPE_UTILITY, SandboxTypeFromString("derp"));
}

}  // namespace sandbox
