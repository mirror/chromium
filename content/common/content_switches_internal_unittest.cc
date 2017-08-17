// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/content_switches_internal.h"
#include "base/command_line.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TEST(ContentSwitchesTest, FeaturesFromSwitch) {
  base::CommandLine cl(base::CommandLine::NO_PROGRAM);
  cl.AppendSwitchASCII("sw", "aaa");
  cl.AppendSwitchASCII("sw", "bb,cc,dd");
  cl.AppendSwitchASCII("sw", "eee");
  EXPECT_EQ(0u, FeaturesFromSwitch(cl, "nope").size());
  auto features = FeaturesFromSwitch(cl, "sw");
  EXPECT_EQ(5u, features.size());
  EXPECT_EQ("aaa", features[0]);
  EXPECT_EQ("bb", features[1]);
  EXPECT_EQ("cc", features[2]);
  EXPECT_EQ("dd", features[3]);
  EXPECT_EQ("eee", features[4]);
}

}  // namespace content
