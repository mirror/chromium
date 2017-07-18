// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/evaluate_capability.h"

#include "base/strings/string_util.h"
#include "remoting/host/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

TEST(EvaluateCapabilityTest, ShouldReturnCrashResult) {
  ASSERT_NE(EvaluateCapabilityAgainstHostBinary(kEvaluateCrash), 0);
}

TEST(EvaluateCapabilityTest, ShouldReturnExitCode) {
  ASSERT_EQ(EvaluateCapabilityAgainstHostBinary(kEvaluateTest), 234);
}

TEST(EvaluateCapabilityTest, ShouldReturnExitCodeAndOutput) {
  std::string output;
  ASSERT_EQ(EvaluateCapabilityAgainstHostBinary(kEvaluateTest, &output), 234);
  // New line character varies on different platform, so normalize the output
  // here.
  base::ReplaceSubstringsAfterOffset(&output, 0, "\r\n", "\n");
  base::ReplaceSubstringsAfterOffset(&output, 0, "\r", "\n");
  ASSERT_EQ("In EvaluateTest(): Line 1\n"
            "In EvaluateTest(): Line 2",
            output);
}

}  // namespace remoting
