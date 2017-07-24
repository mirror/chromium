// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/main_utils.h"

#include <memory>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/process/process_metrics.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace {

// the result via |fnames|. Expects exactly |expected_count|.
bool CheckAndGetFilePathParams(const base::CommandLine& command_line,
                               size_t expected_count,
                               std::vector<base::FilePath>* fnames) {
  const base::CommandLine::StringVector& args = command_line.GetArgs();
  if (args.size() != expected_count)
    return false;

  fnames->clear();
  fnames->reserve(args.size());
  for (const auto& arg : args)
    fnames->emplace_back(arg);
  return true;
}

/******** ResourceUsageTracker ********/

// Tracks and logs system resource usage.
class ResourceUsageTracker {
 public:
  ResourceUsageTracker() = default;
  ~ResourceUsageTracker() = default;

  // Initializes states for tracking, should be called when program starts.
  void Start() { start_time_ = base::Time::Now(); }

  // Computes and prints usage. Should be called at end of program run, e.g.,
  // using base::AtExitManager().
  void Finish() {
    base::Time end_time = base::Time::Now();

#if !defined(OS_MACOSX)
    std::unique_ptr<base::ProcessMetrics> process_metrics(
        base::ProcessMetrics::CreateProcessMetrics(
            base::GetCurrentProcessHandle()));

    LOG(INFO) << "Zucchini.PeakPagefileUsage "
              << process_metrics->GetPeakPagefileUsage() / 1024 << " KiB";
    LOG(INFO) << "Zucchini.PeakWorkingSetSize "
              << process_metrics->GetPeakWorkingSetSize() / 1024 << " KiB";
#endif  // !defined(OS_MACOSX)

    LOG(INFO) << "Zucchini.TotalTime " << (end_time - start_time_).InSecondsF()
              << " s";
  }

 private:
  base::Time start_time_;
};

}  // namespace

namespace command {

/******** Command ********/

Command::Command(const char* const name_in,
                 const char* const usage_in,
                 int num_args_in,
                 CommandFunction* command_function_in)
    : name(name_in),
      usage(usage_in),
      num_args(num_args_in),
      command_function(command_function_in) {}

Command::Command(const Command&) = default;

Command::~Command() = default;

/******** CommandRegistry ********/

CommandRegistry::CommandRegistry(std::ostream* out) : out_(out) {
  // Register all command.
  commands_.push_back({"gen", "-gen <old_file> <new_file> <patch_file>",
                       kGenNumFileParams, Gen});
  commands_.push_back({"apply", "-apply <old_file> <patch_file> <new_file>",
                       kApplyNumFileParams, Apply});
}

CommandRegistry::~CommandRegistry() = default;

zucchini::status::Code CommandRegistry::Run(
    const base::CommandLine& command_line) {
  const Command* command_use = nullptr;
  for (const Command& command : commands_) {
    if (command_line.HasSwitch(command.name)) {
      if (command_use) {        // Too many commands found.
        command_use = nullptr;  // Set to null to flag error.
        break;
      }
      command_use = &command;
    }
  }

  // If we don't have exactly one matching command, print usage and quit.
  if (!command_use) {
    *out_ << "Must have exactly one of:\n  [";
    zucchini::PrefixSep sep(", ");
    for (const Command& command : commands_)
      *out_ << sep << "-" << command.name;
    *out_ << "]" << std::endl;
    PrintUsage();
    return zucchini::status::kStatusMissingCommand;
  }

  // Try to parse filename arguments. On failure, print usage and quit.
  std::vector<base::FilePath> fnames;
  if (!CheckAndGetFilePathParams(command_line, command_use->num_args,
                                 &fnames)) {
    *out_ << command_use->usage << std::endl;
    PrintUsage();
    return zucchini::status::kStatusMissingFiles;
  }

  ResourceUsageTracker resource_usage_tracker;
  resource_usage_tracker.Start();
  zucchini::status::Code exit_code =
      command_use->command_function(command_line, fnames);
  resource_usage_tracker.Finish();
  return exit_code;
}

void CommandRegistry::PrintUsage() {
  *out_ << "Usage:" << std::endl;
  for (const Command& command : commands_)
    *out_ << "  zucchini " << command.usage << std::endl;
}

}  // namespace command
