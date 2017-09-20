// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/sandbox_policy.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TEST(SandboxPolicyTest, Empty) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                 "network");
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));

  EXPECT_FALSE(command_line.HasSwitch(switches::kNoSandbox));
  SetCommandLineFlagsForSandboxPolicy(&command_line, SANDBOX_POLICY_NO_SANDBOX);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
  EXPECT_TRUE(command_line.HasSwitch(switches::kNoSandbox));
}

TEST(SandboxPolicyTest, Renderer) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessPolicy,
                                 switches::kRendererProcess);
  EXPECT_EQ(SANDBOX_POLICY_RENDERER,
            SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                 "network");
  EXPECT_EQ(SANDBOX_POLICY_RENDERER,
            SandboxPolicyFromCommandLine(command_line));

  EXPECT_FALSE(command_line.HasSwitch(switches::kNoSandbox));
  SetCommandLineFlagsForSandboxPolicy(&command_line, SANDBOX_POLICY_NO_SANDBOX);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
  EXPECT_TRUE(command_line.HasSwitch(switches::kNoSandbox));
}

TEST(SandboxPolicyTest, Utility) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessPolicy,
                                 switches::kUtilityProcess);
  EXPECT_EQ(SANDBOX_POLICY_UTILITY, SandboxPolicyFromCommandLine(command_line));

  base::CommandLine command_line2(command_line);
  SetCommandLineFlagsForSandboxPolicy(&command_line2, SANDBOX_POLICY_NETWORK);
  EXPECT_EQ(SANDBOX_POLICY_NETWORK,
            SandboxPolicyFromCommandLine(command_line2));

  base::CommandLine command_line3(command_line);
  SetCommandLineFlagsForSandboxPolicy(&command_line3, SANDBOX_POLICY_WIDEVINE);
  EXPECT_EQ(SANDBOX_POLICY_WIDEVINE,
            SandboxPolicyFromCommandLine(command_line3));

  base::CommandLine command_line4(command_line);
  SetCommandLineFlagsForSandboxPolicy(&command_line4,
                                      SANDBOX_POLICY_NO_SANDBOX);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line4));

  base::CommandLine command_line5(command_line);
  SetCommandLineFlagsForSandboxPolicy(&command_line5, SANDBOX_POLICY_PPAPI);
  EXPECT_EQ(SANDBOX_POLICY_PPAPI, SandboxPolicyFromCommandLine(command_line5));

  base::CommandLine command_line6(command_line);
  command_line6.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                  "bogus");
  EXPECT_EQ(SANDBOX_POLICY_UTILITY,
            SandboxPolicyFromCommandLine(command_line6));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
}

TEST(SandboxPolicyTest, GPU) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessPolicy,
                                 switches::kGpuProcess);
  SetCommandLineFlagsForSandboxPolicy(&command_line, SANDBOX_POLICY_GPU);
  EXPECT_EQ(SANDBOX_POLICY_GPU, SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                 "network");
  EXPECT_EQ(SANDBOX_POLICY_GPU, SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
}

TEST(SandboxPolicyTest, PPAPIBroker) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessPolicy,
                                 switches::kPpapiBrokerProcess);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                 "network");
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
}

TEST(SandboxPolicyTest, PPAPIPlugin) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessPolicy,
                                 switches::kPpapiPluginProcess);
  SetCommandLineFlagsForSandboxPolicy(&command_line, SANDBOX_POLICY_PPAPI);
  EXPECT_EQ(SANDBOX_POLICY_PPAPI, SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                 "network");
  EXPECT_EQ(SANDBOX_POLICY_PPAPI, SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
}

TEST(SandboxPolicyTest, Nonesuch) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessPolicy, "nonesuch");
  EXPECT_EQ(SANDBOX_POLICY_INVALID, SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kUtilityProcessSandboxPolicy,
                                 "network");
  EXPECT_EQ(SANDBOX_POLICY_INVALID, SandboxPolicyFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_POLICY_NO_SANDBOX,
            SandboxPolicyFromCommandLine(command_line));
}

}  // namespace content
