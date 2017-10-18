// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/app/shell_help.h"

#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_split.h"
#include "content/public/common/content_switches.h"
#include "content/shell/common/shell_switches.h"
#include "extensions/common/switches.h"
#include "extensions/shell/common/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

using ShellHelpTest = testing::Test;

namespace extensions {

TEST_F(ShellHelpTest, GetHelpText) {
  base::CommandLine command_line(
      base::FilePath(FILE_PATH_LITERAL("app_shell")));
  std::string help_text = GetHelpText(command_line);

  std::vector<std::string> lines = base::SplitString(
      help_text, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // There should be a bunch of non-empty lines.
  ASSERT_GT(help_text.size(), 5u);

  // First line contains usage information.
  EXPECT_EQ("Usage: app_shell [options] [<file>]", lines[0]);

  // Useful flags are explained.
  const char* expected_switches[] = {
      switches::kLoadApps, switches::kLoadExtension,
      ::switches::kContentShellDataPath, ::switches::kLogFileAoeu,
  };

  for (const char* expected_switch : expected_switches) {
    std::string switch_string = std::string("--") + expected_switch + "=";
    EXPECT_NE(std::string::npos, help_text.find(switch_string));
  }

  // Author information (that's us!) is included.
  EXPECT_NE(std::string::npos, help_text.find("Author"));
}

}  // namespace extensions
