// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_

#include <memory>
#include <ostream>
#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/installer/zucchini/zucchini.h"

// Utilities to register Zucchini commands, to dispatch command based on
// command-line input, to print help messages, and to log system resource usage.

namespace base {

class CommandLine;

}  // namespace base

// Colleciton of static functions and states to to track and log system resource
// usage.
class ResourceUsageTracker {
 public:
  // Initializes states for tracking, should be called when program starts.
  static void Start();

  // Computes and prints usage. Should be called at end of program run, e.g.,
  // using base::AtExitManager().
  static void Finish();

  // Cancels tracking, to prevent Finish() from producing output.
  static void Cancel();

 private:
  // Static States: Should only store pointers or POD.
  static bool is_tracking_;
  static std::unique_ptr<base::Time> start_time_;
};

// Specs for a Zucchini command.
struct Command {
  using Fun = base::Callback<zucchini::status::Code(
      const base::CommandLine&,
      const std::vector<base::FilePath>&)>;

  Command(const char* name_in,
          const char* usage_in,
          int num_args_in,
          Fun fun_in);
  Command(const Command&);
  ~Command();

  // Unique name of command. |-name| is used to select from command line.
  const char* name;

  // Usage help text of command.
  const char* usage;

  // Number of arguments (assumed to be filenames) used by the command.
  const int num_args;

  // Main code for the command.
  Fun fun;
};

// Registry of avaible Commands, along with help text and error handling.
class CommandRegistry {
 public:
  explicit CommandRegistry(std::ostream* out);
  ~CommandRegistry();

  // Registers all accessible commands.
  void RegisterAll();

  // Uses |command_line| to find a Command instance. If a unique command is
  // found, then runs it. Otherwise prints help message (while suppressing
  // resource usage printing). Returns error code (0 = success).
  int Run(const base::CommandLine& command_line);

  // Returns true if Run() actually led to command being called. Otherwise (may
  // happen, e.g., of command line parsing fails) returns false.
  bool DidRunCommand() const { return did_run_command_; }

 private:
  void PrintUsage();

  std::ostream* out_;

  std::vector<Command> commands_;

  bool did_run_command_ = false;

  DISALLOW_COPY_AND_ASSIGN(CommandRegistry);
};

#endif  // CHROME_INSTALLER_ZUCCHINI_MAIN_UTILS_H_
