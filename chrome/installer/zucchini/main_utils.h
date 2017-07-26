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

// Registry of avaible Commands, along with help text and error handling.
class CommandRegistry {
 public:
  explicit CommandRegistry(std::ostream* out);
  ~CommandRegistry();

  // Registers |command| for processing and help text display.
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

#endif  // CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_
