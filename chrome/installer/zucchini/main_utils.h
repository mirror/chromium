// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_

#include <ostream>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "chrome/installer/zucchini/zucchini.h"
#include "chrome/installer/zucchini/zucchini_commands.h"

// Utilities to register Zucchini commands, to dispatch command based on
// command-line input, and to print help messages.

namespace base {

class CommandLine;

}  // namespace base

// Specifications for a Zucchini command.
struct Command {
  using CommandFunction =
      zucchini::status::Code (*)(const base::CommandLine&,
                                 const std::vector<base::FilePath>&);

  // Constructor is constexpr to allow static instances of Command.
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

// Registry of avaible Commands, along with help text and error handling.
class CommandRegistry {
 public:
  explicit CommandRegistry(std::ostream* out);
  ~CommandRegistry();

  void Add(const Command& command);

  // Uses |command_line| to find a Command instance. If a unique command is
  // found, then runs it. Otherwise prints help message (while suppressing
  // resource usage printing). Returns Zucchini exit code for error handling.
  zucchini::status::Code Run(const base::CommandLine& command_line);

 private:
  void PrintUsage();

  std::ostream* out_;

  std::vector<Command> commands_;

  DISALLOW_COPY_AND_ASSIGN(CommandRegistry);
};

/******** Command Declarations ********/
// We can have static instances of Command since its ctor is constexpr.

// Command: Patch generation.
extern Command kCommandGen;

// Command: Patch application.
extern Command kCommandApply;

#endif  // CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_
