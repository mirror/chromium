// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/app/shell_help.h"

#include <algorithm>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/content_switches.h"
#include "content/shell/common/shell_switches.h"
#include "extensions/common/switches.h"
#include "extensions/shell/common/switches.h"

namespace extensions {

namespace {

// Description of a command-line option.
struct CommandLineOption {
  const char* switch_name;
  const char* switch_value;  // Template describing valid value, e.g. "<DIR>".
  const char* description;
};

// Important options to describe. Note that the descriptions will not be
// word-wrapped.
constexpr CommandLineOption kHelpTextOptions[] = {
    {switches::kLoadApps, "<DIR>[,<DIR>...]",
     "App(s) to load. The first app is launched."},
    {switches::kLoadExtension, "<DIR>[,<DIR>...]", "Extension(s) to load."},
    {::switches::kContentShellDataPath, "<DIR>",
     "Path to the directory for config data."},
    {::switches::kLogFileAoeu, "<FILE>", "Location for log output."},
};

constexpr char kUsageLineFormat[] = "Usage: %s [options] [<file>]\n";

// Usage and option notes.
constexpr char kHelpTextUsageNotes[] =
    "  Apps and extensions must be located in unpacked directories.\n"
    "  If <file> is specified, its contents will be provided to the launched "
    "app.\n";

// Trailing text.
constexpr char kHelpTextFooter[] =
    "Author: The Chromium team - <http://www.chromium.org>\n"
    "\n"
    "Bug tracker: <http://crbug.com> (Component: Platform>Apps>Shell)\n";

}  // namespace

std::string GetHelpText(const base::CommandLine& command_line) {
  // Longest switch usage length, used for padding.
  size_t max_switch_name_len = 0;
  for (const auto& option : kHelpTextOptions) {
    // Calculate size of formatted option name. Add 3 for "--" and "=".
    const size_t option_len =
        strlen(option.switch_name) + strlen(option.switch_value) + 3;
    max_switch_name_len = std::max(max_switch_name_len, option_len);
  }

  // In the unlikely event of a non-ASCII filename, just hard-code app_shell.
  std::string exe_name = command_line.GetProgram().BaseName().MaybeAsASCII();
  if (exe_name.empty())
    exe_name = "app_shell";

  std::string help_text;

  // Usage line with executable name.
  base::StringAppendF(&help_text, kUsageLineFormat, exe_name.c_str());
  help_text.push_back('\n');

  // Format string for each option line.
  const std::string option_line_format =
      "  %-" + std::to_string(max_switch_name_len + 2) + "s%s\n";
  for (const auto& option : kHelpTextOptions) {
    base::StringAppendF(
        &help_text, option_line_format.c_str(),
        base::StringPrintf("--%s=%s", option.switch_name, option.switch_value)
            .c_str(),
        option.description);
  }

  help_text.push_back('\n');
  help_text.append(kHelpTextUsageNotes);

  help_text.push_back('\n');
  help_text.append(kHelpTextFooter);

  return help_text;
}

}  // namespace extensions
