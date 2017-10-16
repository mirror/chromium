// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_APP_SHELL_HELP_H_
#define EXTENSIONS_SHELL_APP_SHELL_HELP_H_

#include <string>

namespace base {
class CommandLine;
}  // namespace base

namespace extensions {

// Returns help text suitable for printing in response to --help.
std::string GetHelpText(const base::CommandLine& command_line);

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_APP_SHELL_HELP_H_
