// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "extensions/common/switches.h"
#include "extensions/shell/test/shell_apitest.h"

namespace extensions {

namespace {

const char kTestExtensionId[] = "leagbaipaanbjeooclcojabpocidnlga";

}  // namespace

class MojoPrivateApiTest : public ShellApiTest {
 public:
  void SetUpOnMainThread() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kWhitelistedExtensionID, kTestExtensionId);
    ShellApiTest::SetUpOnMainThread();
  }
};

IN_PROC_BROWSER_TEST_F(MojoPrivateApiTest, NativeMojoBindingsAvailable) {
  ASSERT_TRUE(RunAppTest("api_test/mojo_private")) << message_;
}

}  // namespace extensions
