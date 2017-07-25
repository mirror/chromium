// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_
#define CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_

#include <vector>

#include "base/files/file_path.h"
#include "chrome/installer/zucchini/zucchini.h"

// Zucchini commands and tools that can be invoked from command-line.

namespace base {

class CommandLine;

}  // namespace base

// To add a new Zucchini command:
// (1) Declare the command here (signature must match Command::CommandFunction).
// (2) Define the command in the .cc file.
// (3) Declare / define a Command instance in main_utils.h / main_utils.cc.
// (4) Add the Command to |registry| in main().

// Command: Patch generation.
zucchini::status::Code MainGen(const base::CommandLine& command_line,
                               const std::vector<base::FilePath>& fnames);

// Command: Patch application.
zucchini::status::Code MainApply(const base::CommandLine& command_line,
                                 const std::vector<base::FilePath>& fnames);

#endif  // CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_
