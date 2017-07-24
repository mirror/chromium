// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_
#define CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_

#include <vector>

#include "base/files/file_path.h"
#include "chrome/installer/zucchini/zucchini.h"

// Zucchini commands and tools that can be invoked from the command-line.

namespace base {

class CommandLine;

}  // namespace base

namespace command {

// To add a new Zucchini command:
// (1) Declare the command here, with associated constants.
// (2) Define the command in the .cc file.
// (3) Add an entry in CommandRegistry::RegisterAll().

// Command: Patch generation.
zucchini::status::Code Gen(const base::CommandLine& command_line,
                           const std::vector<base::FilePath>& fnames);
constexpr int kGenNumFileParams = 3;

// Command: Patch application.
zucchini::status::Code Apply(const base::CommandLine& command_line,
                             const std::vector<base::FilePath>& fnames);
constexpr int kApplyNumFileParams = 3;

}  // namespace command

#endif  // CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_
