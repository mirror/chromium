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
// 1. Add the command's main function in Command Function Declarations
//    section below. Its signature must match Command::CommandFunction).
// 2. Define the command's main function in the .cc file.
// 3. Add a Command instance in "Command Definitions" section below.
// 4. Add the Command to |registry| in main().

/******** Command Function Declarations ********/

// Command: Patch generation.
zucchini::status::Code MainGen(const base::CommandLine& command_line,
                               const std::vector<base::FilePath>& fnames);

// Command: Patch application.
zucchini::status::Code MainApply(const base::CommandLine& command_line,
                                 const std::vector<base::FilePath>& fnames);

/******** Command ********/

// Specifications for a Zucchini command.
struct Command {
  using CommandFunction =
      zucchini::status::Code (*)(const base::CommandLine&,
                                 const std::vector<base::FilePath>&);

  constexpr Command(const char* const name_in,
                    const char* const usage_in,
                    int num_args_in,
                    CommandFunction command_function_in)
      : name(name_in),
        usage(usage_in),
        num_args(num_args_in),
        command_function(command_function_in) {}
  Command(const Command&) = default;
  ~Command() = default;

  // Unique name of command. |-name| is used to select from command-line.
  const char* const name;

  // Usage help text of command.
  const char* const usage;

  // Number of arguments (assumed to be filenames) used by the command.
  const int num_args;

  // Main code for the command.
  const CommandFunction command_function;
};

/******** Command Specifications ********/

// Command: Patch generation.
constexpr Command kCommandGen("gen",
                              "-gen <old_file> <new_file> <patch_file>",
                              3,
                              MainGen);

// Command: Patch application.
constexpr Command kCommandApply("apply",
                                "-apply <old_file> <patch_file> <new_file>",
                                3,
                                MainApply);

#endif  // CHROME_INSTALLER_ZUCCHINI_ZUCCHINI_COMMANDS_H_
