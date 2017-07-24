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

namespace command {

struct Command {
  Command(const char* const name_in,
          const char* const usage_in,
          int num_args_in,
          CommandFunction* command_function);
  Command(const Command&);
  ~Command();

  // Unique name of command. |-name| is used to select from command line.
  const char* const name;

  // Usage help text of command.
  const char* const usage;

  // Number of arguments (assumed to be filenames) used by the command.
  const int num_args;

  // Main code for the command.
  CommandFunction* command_function;
};

// Registry of avaible Commands, along with help text and error handling.
class CommandRegistry {
 public:
  explicit CommandRegistry(std::ostream* out);
  ~CommandRegistry();

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

}  // namespace command

#endif  // CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_
